#pragma once

#include <fstream>
#include <cstdio>
#include <vector>
#include <queue>
#include <limits>
#include <cmath>

#include "L2space.h"
#include "hnswalg.h"
#include <faiss/ProductQuantizer.h>
#include <faiss/utils.h>
#include <faiss/index_io.h>

typedef unsigned int idx_t;
typedef unsigned char uint8_t;

class StopW {
    std::chrono::steady_clock::time_point time_begin;
public:
    StopW() {
        time_begin = std::chrono::steady_clock::now();
    }
    float getElapsedTimeMicro() {
        std::chrono::steady_clock::time_point time_end = std::chrono::steady_clock::now();
        return (std::chrono::duration_cast<std::chrono::microseconds>(time_end - time_begin).count());
    }
    void reset() {
        time_begin = std::chrono::steady_clock::now();
    }

};

namespace hnswlib {

    struct ModifiedIndex
	{
		size_t d = 96;             /** Vector Dimension **/
		size_t nc = 999973;            /** Number of Centroids **/
        size_t nsubc;         /** Number of Subcentroids **/
        size_t code_size = 16;     /** PQ Code Size **/

        /** Query members **/
        size_t nprobe = 16;
        size_t max_codes = 10000;

        /** NEW **/
        std::vector < std::vector < idx_t > > ids;
        std::vector < std::vector < uint8_t > > codes;
        std::vector < std::vector < uint8_t > > norm_codes;
        std::vector < std::vector < idx_t > > nn_centroid_idxs;
        std::vector < std::vector < idx_t > > group_sizes;
        std::vector < float > alphas;

        faiss::ProductQuantizer *norm_pq;
        faiss::ProductQuantizer *pq;

        HierarchicalNSW<float, float> *quantizer;

        std::vector< std::vector<float> > s_c;
    public:

		ModifiedIndex(size_t dim, size_t ncentroids, size_t bytes_per_code,
                    size_t nbits_per_idx, size_t nsubcentroids = 64):
                 d(dim), nc(ncentroids), nsubc(nsubcentroids)
		{
            codes.resize(nc);
            norm_codes.resize(nc);
            ids.resize(nc);
            alphas.resize(nc);
            nn_centroid_idxs.resize(nc);
            group_sizes.resize(nc);

            pq = new faiss::ProductQuantizer(d, bytes_per_code, nbits_per_idx);
            norm_pq = new faiss::ProductQuantizer(1, 1, nbits_per_idx);

            dis_table.resize(pq->ksub * pq->M);
            norms.resize(65536);
            code_size = pq->code_size;

            /** Compute centroid norms **/
            centroid_norms.resize(nc);
            s_c.resize(nc);
        }


		~ModifiedIndex()
        {
            delete pq;
            delete norm_pq;
            delete quantizer;
        }

        void buildQuantizer(SpaceInterface<float> *l2space, const char *path_clusters,
                            const char *path_info, const char *path_edges, int efSearch)
        {
            if (exists_test(path_info) && exists_test(path_edges)) {
                quantizer = new HierarchicalNSW<float, float>(l2space, path_info, path_clusters, path_edges);
                quantizer->ef_ = efSearch;
                return;
            }
            quantizer = new HierarchicalNSW<float, float>(l2space, nc, 16, 32, 500);
            quantizer->ef_ = efSearch;

            std::cout << "Constructing quantizer\n";
            int j1 = 0;
            std::ifstream input(path_clusters, ios::binary);

            float mass[d];
            readXvec<float>(input, mass, d);
            quantizer->addPoint((void *) (mass), j1);

            size_t report_every = 100000;
            #pragma omp parallel for num_threads(16)
            for (int i = 1; i < nc; i++) {
                float mass[d];
                #pragma omp critical
                {
                    readXvec<float>(input, mass, d);
                    if (++j1 % report_every == 0)
                        std::cout << j1 / (0.01 * nc) << " %\n";
                }
                quantizer->addPoint((void *) (mass), (size_t) j1);
            }
            input.close();
            quantizer->SaveInfo(path_info);
            quantizer->SaveEdges(path_edges);
        }


        void assign(size_t n, const float *data, idx_t *idxs)
        {
            #pragma omp parallel for num_threads(18)
            for (int i = 0; i < n; i++)
                idxs[i] = quantizer->searchKnn(const_cast<float *>(data + i*d), 1).top().second;
        }

        void add(const char *path_groups, const char *path_idxs)
        {
            size_t maxM = 32;
            StopW stopw = StopW();

            double baseline_average = 0.0;
            double modified_average = 0.0;

            std::ifstream input_groups(path_groups, ios::binary);
            std::ifstream input_idxs(path_idxs, ios::binary);

            /** Vectors for construction **/
            std::vector< std::vector<float> > centroid_vector_norms_L2sqr(nc);

            /** Find NN centroids to source centroid **/
            std::cout << "Find NN centroids to source centroids\n";

            #pragma omp parallel for num_threads(20)
            for (int i = 0; i < nc; i++) {
                const float *centroid = (float *) quantizer->getDataByInternalId(i);
                std::priority_queue<std::pair<float, idx_t>> nn_centroids_raw = quantizer->searchKnn((void *) centroid, nsubc + 1);

                /** Pruning **/
//                std::priority_queue<std::pair<float, idx_t>> heuristic_nn_centroids;
//                while (nn_centroids_raw.size() > 1) {
//                    heuristic_nn_centroids.emplace(nn_centroids_raw.top());
//                    nn_centroids_raw.pop();
//                }
//                quantizer->getNeighborsByHeuristicMerge(heuristic_nn_centroids, maxM);
//
//                centroid_vector_norms_L2sqr[i].resize(nsubc);
//                nn_centroid_idxs[i].resize(nsubc);
//                while (heuristic_nn_centroids.size() > 0) {
//                    centroid_vector_norms_L2sqr[i][heuristic_nn_centroids.size() - 1] = heuristic_nn_centroids.top().first;
//                    nn_centroid_idxs[i][heuristic_nn_centroids.size() - 1] = heuristic_nn_centroids.top().second;
//                    heuristic_nn_centroids.pop();
//                }
                /** Without heuristic **/
                centroid_vector_norms_L2sqr[i].resize(nsubc);
                nn_centroid_idxs[i].resize(nsubc);
                while (nn_centroids_raw.size() > 1) {
                    centroid_vector_norms_L2sqr[i][nn_centroids_raw.size() - 2] = nn_centroids_raw.top().first;
                    nn_centroid_idxs[i][nn_centroids_raw.size() - 2] = nn_centroids_raw.top().second;
                    nn_centroids_raw.pop();
                }
            }

            /** Adding groups to index **/
            std::cout << "Adding groups to index\n";
            int j1 = 0;
            #pragma omp parallel for reduction(+:baseline_average, modified_average) num_threads(20)
            for (int c = 0; c < nc; c++) {
                /** Read Original vectors from Group file**/
                idx_t centroid_num;
                int groupsize;
                std::vector<float> data;
                std::vector<idx_t> idxs;

                #pragma omp critical
                {
                    input_groups.read((char *) &groupsize, sizeof(int));
                    input_idxs.read((char *) &groupsize, sizeof(int));

                    data.resize(groupsize * d);
                    idxs.resize(groupsize);

                    input_groups.read((char *) data.data(), groupsize * d * sizeof(float));
                    input_idxs.read((char *) idxs.data(), groupsize * sizeof(idx_t));

                    centroid_num = j1++;
                    if (j1 % 10000 == 0) {
                        std::cout << "[" << stopw.getElapsedTimeMicro() / 1000000 << "s] "
                                  << (100. * j1) / 1000000 << "%" << std::endl;
                    }
                }

                if (groupsize == 0)
                    continue;

                const float *centroid = (float *) quantizer->getDataByInternalId(centroid_num);
                const float *centroid_vector_norms = centroid_vector_norms_L2sqr[centroid_num].data();
                const idx_t *nn_centroids = nn_centroid_idxs[centroid_num].data();

                /** Compute centroid-neighbor_centroid and centroid-group_point vectors **/
                std::vector<float> centroid_vectors(nsubc * d);
                for (int subc = 0; subc < nsubc; subc++) {
                    float *neighbor_centroid = (float *) quantizer->getDataByInternalId(nn_centroids[subc]);
                    sub_vectors(centroid_vectors.data() + subc * d, neighbor_centroid, centroid);
                }

                /** Find alphas for vectors **/
                alphas[centroid_num] = compute_alpha(centroid_vectors.data(), data.data(), centroid,
                                                     centroid_vector_norms, groupsize);

                /** Compute final subcentroids **/
                std::vector<float> subcentroids(nsubc * d);
                for (int subc = 0; subc < nsubc; subc++) {
                    const float *centroid_vector = centroid_vectors.data() + subc * d;
                    float *subcentroid = subcentroids.data() + subc * d;
                    faiss::fvec_madd (d, centroid, alphas[centroid_num], centroid_vector, subcentroid);
                }

                /** Find subcentroid idx **/
                std::vector<idx_t> subcentroid_idxs(groupsize);
                compute_subcentroid_idxs(subcentroid_idxs.data(), subcentroids.data(), data.data(), groupsize);

                /** Compute Residuals **/
                std::vector<float> residuals(groupsize*d);
                compute_residuals(groupsize, residuals.data(), data.data(), subcentroids.data(), subcentroid_idxs.data());

                /** Compute Codes **/
                std::vector<uint8_t> xcodes(groupsize * code_size);
                pq->compute_codes(residuals.data(), xcodes.data(), groupsize);

                /** Decode Codes **/
                std::vector<float> decoded_residuals(groupsize*d);
                pq->decode(xcodes.data(), decoded_residuals.data(), groupsize);

                /** Reconstruct Data **/
                std::vector<float> reconstructed_x(groupsize*d);
                reconstruct(groupsize, reconstructed_x.data(), decoded_residuals.data(),
                            subcentroids.data(), subcentroid_idxs.data());

                /** Compute norms **/
                std::vector<float> norms(groupsize);
                faiss::fvec_norms_L2sqr (norms.data(), reconstructed_x.data(), d, groupsize);

                /** Compute norm codes **/
                std::vector<uint8_t > xnorm_codes(groupsize);
                norm_pq->compute_codes(norms.data(), xnorm_codes.data(), groupsize);

                /** Distribute codes **/
                std::vector < std::vector<idx_t> > construction_ids(nsubc);
                std::vector < std::vector<uint8_t> > construction_codes(nsubc);
                std::vector < std::vector<uint8_t> > construction_norm_codes(nsubc);
                for (int i = 0; i < groupsize; i++) {
                    const idx_t idx = idxs[i];
                    const idx_t subcentroid_idx = subcentroid_idxs[i];

                    construction_ids[subcentroid_idx].push_back(idx);
                    construction_norm_codes[subcentroid_idx].push_back(xnorm_codes[i]);
                    for (int j = 0; j < code_size; j++)
                        construction_codes[subcentroid_idx].push_back(xcodes[i * code_size + j]);

                    const float *subcentroid = subcentroids.data() + subcentroid_idx * d;
                    const float *point = data.data() + i * d;
                    baseline_average += faiss::fvec_L2sqr(centroid, point, d);
                    modified_average += faiss::fvec_L2sqr(subcentroid, point, d);
                }
                /** Add codes **/
                for (int subc = 0; subc < nsubc; subc++) {
                    idx_t subcsize = construction_norm_codes[subc].size();
                    group_sizes[centroid_num].push_back(subcsize);

                    for (int i = 0; i < subcsize; i++) {
                        ids[centroid_num].push_back(construction_ids[subc][i]);
                        for (int j = 0; j < code_size; j++)
                            codes[centroid_num].push_back(construction_codes[subc][i * code_size + j]);
                        norm_codes[centroid_num].push_back(construction_norm_codes[subc][i]);
                    }
                }
            }
            std::cout << "[Baseline] Average Distance: " << baseline_average / 1000000000 << std::endl;
            std::cout << "[Modified] Average Distance: " << modified_average / 1000000000 << std::endl;

            input_groups.close();
            input_idxs.close();
        }

		void search(float *x, idx_t k, idx_t *results)
		{
            idx_t keys[nprobe];
            float q_c[nprobe];

            pq->compute_inner_prod_table(x, dis_table.data());
            //std::priority_queue<std::pair<float, idx_t>, std::vector<std::pair<float, idx_t>>, CompareByFirst> topResults;
            std::priority_queue<std::pair<float, idx_t>> topResults;

            auto coarse = quantizer->searchKnn(x, nprobe);
            for (int i = nprobe - 1; i >= 0; i--) {
                auto elem = coarse.top();
                q_c[i] = elem.first;
                keys[i] = elem.second;
                coarse.pop();
            }

            for (int i = 0; i < nprobe; i++){
                idx_t centroid_num = keys[i];
                if (group_sizes[centroid_num].size() == 0)
                    continue;
                norm_pq->decode(norm_codes[centroid_num].data(), norms.data(), norm_codes[centroid_num].size());

                const uint8_t *code = codes[centroid_num].data();
                const float *norm = norms.data();
                const idx_t *id = ids[centroid_num].data();
                const idx_t *nn_centroids = nn_centroid_idxs[centroid_num].data();
                float alpha = alphas[centroid_num];

                const float *centroid = (float *) quantizer->getDataByInternalId(centroid_num);
                float fst_term = (1 - alpha) * (q_c[i] - centroid_norms[centroid_num]);

                std::vector< float > q_s(nsubc);
                std::vector< float > r(nsubc);

                /** Filtering **/
                std::priority_queue<std::pair<float, idx_t>, std::vector<std::pair<float, idx_t>>, CompareByFirst> ordered_subc;
                for (int subc = 0; subc < nsubc; subc++) {
                    idx_t subcentroid_num = nn_centroids[subc];
                    const float *nn_centroid = (float *) quantizer->getDataByInternalId(subcentroid_num);

                    q_s[subc] = faiss::fvec_L2sqr(x, nn_centroid, d);
                    r[subc] = (1-alpha) * q_c[i] + alpha * (alpha-1) * s_c[centroid_num][subc] + alpha * q_s[subc];

                    ordered_subc.emplace(std::make_pair(-r[subc], subc));
                }

                //double average_r = 0.0;
                //for (int subc = 0; subc < nsubc; subc++) {
                //    idx_t subcentroid_num = nn_centroids[subc];
                //    const float *nn_centroid = (float *) quantizer->getDataByInternalId(subcentroid_num);

                //    q_s[subc] = faiss::fvec_L2sqr(x, nn_centroid, d);
                    //r[subc] = (1-alpha) * q_c[i] + alpha * (alpha-1) * s_c[centroid_num][subc] + alpha * q_s[subc];
                    //r[subc] = (1+alpha) * q_c[i] + alpha * (alpha+1) * s_c[centroid_num][subc] - alpha * q_s[subc];
                    //average_r += r[subc];
                //}
                //average_r /= nsubc;

                while (ordered_subc.size() > nprobe/2){
                    idx_t subc = ordered_subc.top().second;
                    ordered_subc.pop();
                //for (int subc = 0; subc < nsubc; subc++){
                    int groupsize = group_sizes[centroid_num][subc];
                    if (groupsize == 0)
                        continue;

//                    if (r[subc] > average_r) {
//                        code += groupsize*code_size;
//                        norm += groupsize;
//                        id += groupsize;
//                        continue;
//                    }

                    idx_t subcentroid_num = nn_centroids[subc];
                    const float *nn_centroid = (float *) quantizer->getDataByInternalId(subcentroid_num);
                    float q_s = faiss::fvec_L2sqr(x, nn_centroid, d);
                    float snd_term = alpha * (q_s - centroid_norms[subcentroid_num]);

                    for (int j = 0; j < groupsize; j++){
                        float q_r = fstdistfunc(const_cast<uint8_t *>(code)+ j*code_size);
                        float dist = fst_term + snd_term - 2*q_r + norm[j];
                        topResults.emplace(std::make_pair(-dist, id[j]));
                    }
                    /** Shift to the next group **/
                    code += groupsize*code_size;
                    norm += groupsize;
                    id += groupsize;
                }
                if (topResults.size() >= max_codes)
                    break;
            }

            for (int i = 0; i < k; i++) {
                results[i] = topResults.top().second;
                topResults.pop();
            }
		}

        void write(const char *path_index)
        {
            FILE *fout = fopen(path_index, "wb");

            fwrite(&d, sizeof(size_t), 1, fout);
            fwrite(&nc, sizeof(size_t), 1, fout);
            fwrite(&nsubc, sizeof(size_t), 1, fout);

            idx_t size;
            /** Save Vector Indexes per  **/
            for (size_t i = 0; i < nc; i++) {
                size = ids[i].size();
                fwrite(&size, sizeof(idx_t), 1, fout);
                fwrite(ids[i].data(), sizeof(idx_t), size, fout);
            }
            /** Save PQ Codes **/
            for(int i = 0; i < nc; i++) {
                size = codes[i].size();
                fwrite(&size, sizeof(idx_t), 1, fout);
                fwrite(codes[i].data(), sizeof(uint8_t), size, fout);
            }
            /** Save Norm Codes **/
            for(int i = 0; i < nc; i++) {
                size = norm_codes[i].size();
                fwrite(&size, sizeof(idx_t), 1, fout);
                fwrite(norm_codes[i].data(), sizeof(uint8_t), size, fout);
            }
            /** Save NN Centroid Indexes **/
            for(int i = 0; i < nc; i++) {
                size = nn_centroid_idxs[i].size();
                fwrite(&size, sizeof(idx_t), 1, fout);
                fwrite(nn_centroid_idxs[i].data(), sizeof(idx_t), size, fout);
            }
            /** Write Group Sizes **/
            for(int i = 0; i < nc; i++) {
                size = group_sizes[i].size();
                fwrite(&size, sizeof(idx_t), 1, fout);
                fwrite(group_sizes[i].data(), sizeof(idx_t), size, fout);
            }
            /** Save Alphas **/
            fwrite(alphas.data(), sizeof(float), nc, fout);
            fclose(fout);
        }

        void read(const char *path_index)
        {
            FILE *fin = fopen(path_index, "rb");

            fread(&d, sizeof(size_t), 1, fin);
            fread(&nc, sizeof(size_t), 1, fin);
            fread(&nsubc, sizeof(size_t), 1, fin);

            ids.resize(nc);
            codes.resize(nc);
            norm_codes.resize(nc);
            alphas.resize(nc);
            nn_centroid_idxs.resize(nc);
            group_sizes.resize(nc);

            idx_t size;
            /** Read Indexes **/
            for (size_t i = 0; i < nc; i++) {
                fread(&size, sizeof(idx_t), 1, fin);
                ids[i].resize(size);
                fread(ids[i].data(), sizeof(idx_t), size, fin);
            }
            /** Read Codes **/
            for(size_t i = 0; i < nc; i++) {
                fread(&size, sizeof(idx_t), 1, fin);
                codes[i].resize(size);
                fread(codes[i].data(), sizeof(uint8_t), size, fin);
            }
            /** Read Norm Codes **/
            for(size_t i = 0; i < nc; i++) {
                fread(&size, sizeof(idx_t), 1, fin);
                norm_codes[i].resize(size);
                fread(norm_codes[i].data(), sizeof(uint8_t), size, fin);
            }
            /** Read NN Centroid Indexes **/
            for(size_t i = 0; i < nc; i++) {
                fread(&size, sizeof(idx_t), 1, fin);
                nn_centroid_idxs[i].resize(size);
                fread(nn_centroid_idxs[i].data(), sizeof(idx_t), size, fin);
            }
            /** Read Group Sizes **/
            for(size_t i = 0; i < nc; i++) {
                fread(&size, sizeof(idx_t), 1, fin);
                group_sizes[i].resize(size);
                fread(group_sizes[i].data(), sizeof(idx_t), size, fin);
            }

            fread(alphas.data(), sizeof(float), nc, fin);
            fclose(fin);
        }

        void train_residual_pq(const size_t n, const float *x) {
            std::vector<float> train_residuals;
            std::vector<idx_t> assigned(n);
            assign(n, x, assigned.data());

            std::unordered_map<idx_t, std::vector<float>> group_map;

            for (int i = 0; i < n; i++) {
                idx_t key = assigned[i];
                for (int j = 0; j < d; j++)
                    group_map[key].push_back(x[i * d + j]);
            }

            for (auto p : group_map) {
                const idx_t centroid_num = p.first;
                const float *centroid = (float *) quantizer->getDataByInternalId(centroid_num);
                const vector<float> data = p.second;
                const int groupsize = data.size() / d;

                std::vector<idx_t> nn_centroids(nsubc);
                std::vector<float> centroid_vector_norms(nsubc);
                auto nn_centroids_raw = quantizer->searchKnn((void *) centroid, nsubc + 1);

                while (nn_centroids_raw.size() > 1) {
                    centroid_vector_norms[nn_centroids_raw.size() - 2] = nn_centroids_raw.top().first;
                    nn_centroids[nn_centroids_raw.size() - 2] = nn_centroids_raw.top().second;
                    nn_centroids_raw.pop();
                }

                /** Compute centroid-neighbor_centroid and centroid-group_point vectors **/
                std::vector<float> centroid_vectors(nsubc * d);
                for (int i = 0; i < nsubc; i++) {
                    const float *neighbor_centroid = (float *) quantizer->getDataByInternalId(nn_centroids[i]);
                    sub_vectors(centroid_vectors.data() + i * d, neighbor_centroid, centroid);
                }

                /** Find alphas for vectors **/
                float alpha = compute_alpha(centroid_vectors.data(), data.data(), centroid,
                                            centroid_vector_norms.data(), groupsize);

                /** Compute final subcentroids **/
                std::vector<float> subcentroids(nsubc * d);
                for (int subc = 0; subc < nsubc; subc++) {
                    const float *centroid_vector = centroid_vectors.data() + subc * d;
                    float *subcentroid = subcentroids.data() + subc * d;

                    faiss::fvec_madd (d, centroid, alpha, centroid_vector, subcentroid);
                }

                /** Find subcentroid idx **/
                std::vector<idx_t> subcentroid_idxs(groupsize);
                compute_subcentroid_idxs(subcentroid_idxs.data(), subcentroids.data(), data.data(), groupsize);

                /** Compute Residuals **/
                std::vector<float> residuals(groupsize * d);
                compute_residuals(groupsize, residuals.data(), data.data(), subcentroids.data(),
                                  subcentroid_idxs.data());

                for (int i = 0; i < groupsize; i++)
                    for (int j = 0; j < d; j++)
                        train_residuals.push_back(residuals[i * d + j]);
            }

            printf("Training %zdx%zd product quantizer on %ld vectors in %dD\n",
                   pq->M, pq->ksub, train_residuals.size()/d, d);
            pq->verbose = true;
            pq->train(n, train_residuals.data());
        }

        void train_norm_pq (const size_t n, const float *x)
        {
            std::vector<float> train_norms;

            std::vector<idx_t> assigned(n);
            assign(n, x, assigned.data());

            std::unordered_map<idx_t, std::vector<float>> group_map;

            for (int i = 0; i < n; i++) {
                idx_t key = assigned[i];
                for (int j = 0; j < d; j++)
                    group_map[key].push_back(x[i * d + j]);
            }

            for (auto p : group_map) {
                const idx_t centroid_num = p.first;
                const float *centroid = (float *) quantizer->getDataByInternalId(centroid_num);
                const vector<float> data = p.second;
                const int groupsize = data.size() / d;

                std::vector<idx_t> nn_centroids(nsubc);
                std::vector<float> centroid_vector_norms(nsubc);
                std::priority_queue<std::pair<float, idx_t>> nn_centroids_raw = quantizer->searchKnn((void *) centroid, nsubc + 1);

                while (nn_centroids_raw.size() > 1) {
                    centroid_vector_norms[nn_centroids_raw.size() - 2] = nn_centroids_raw.top().first;
                    nn_centroids[nn_centroids_raw.size() - 2] = nn_centroids_raw.top().second;
                    nn_centroids_raw.pop();
                }

                /** Compute centroid-neighbor_centroid and centroid-group_point vectors **/
                std::vector<float> centroid_vectors(nsubc * d);
                for (int i = 0; i < nsubc; i++) {
                    const float *neighbor_centroid = (float *) quantizer->getDataByInternalId(nn_centroids[i]);
                    sub_vectors(centroid_vectors.data() + i * d, neighbor_centroid, centroid);
                }

                /** Find alphas for vectors **/
                float alpha = compute_alpha(centroid_vectors.data(), data.data(), centroid,
                                            centroid_vector_norms.data(), groupsize);

                /** Compute final subcentroids **/
                std::vector<float> subcentroids(nsubc * d);
                for (int subc = 0; subc < nsubc; subc++) {
                    const float *centroid_vector = centroid_vectors.data() + subc * d;
                    float *subcentroid = subcentroids.data() + subc * d;

                    faiss::fvec_madd (d, centroid, alpha, centroid_vector, subcentroid);
                }

                /** Find subcentroid idx **/
                std::vector<idx_t> subcentroid_idxs(groupsize);
                compute_subcentroid_idxs(subcentroid_idxs.data(), subcentroids.data(), data.data(), groupsize);

                /** Compute Residuals **/
                std::vector<float> residuals(groupsize * d);
                compute_residuals(groupsize, residuals.data(), data.data(), subcentroids.data(),
                                  subcentroid_idxs.data());

                /** Compute Codes **/
                std::vector<uint8_t> xcodes(groupsize *code_size);
                pq->compute_codes(residuals.data(), xcodes.data(), groupsize);

                /** Decode Codes **/
                std::vector<float> decoded_residuals(groupsize *d);
                pq->decode(xcodes.data(), decoded_residuals.data(), groupsize);

                /** Reconstruct Data **/
                std::vector<float> reconstructed_x(groupsize *d);
                reconstruct(groupsize, reconstructed_x.data(), decoded_residuals.data(),
                            subcentroids.data(), subcentroid_idxs.data());

                /** Compute norms **/
                std::vector<float> group_norms(groupsize);
                faiss::fvec_norms_L2sqr(group_norms.data(), reconstructed_x.data(), d, groupsize);

                for (int i = 0; i < groupsize; i++)
                    train_norms.push_back(group_norms[i]);
            }
            printf("Training %zdx%zd product quantizer on %ld vectors in 1D\n",
                   norm_pq->M, norm_pq->ksub, train_norms.size());
            norm_pq->verbose = true;
            norm_pq->train(n, train_norms.data());
        }

        void compute_centroid_norms()
        {
            #pragma omp parallel for num_threads(16)
            for (int i = 0; i < nc; i++){
                const float *centroid = (float *)quantizer->getDataByInternalId(i);
                centroid_norms[i] = faiss::fvec_norm_L2sqr (centroid, d);
            }
        }

        void compute_s_c() {
            for (int i = 0; i < nc; i++){
                const float *centroid = (float *) quantizer->getDataByInternalId(i);
                s_c[i].resize(nsubc);
                for (int subc = 0; subc < nsubc; subc++){
                    idx_t subc_idx = nn_centroid_idxs[i][subc];
                    const float *subcentroid = (float *) quantizer->getDataByInternalId(subc_idx);
                    s_c[i][subc] = faiss::fvec_L2sqr(subcentroid, centroid, d);
                }
            }
        }

        void compute_graphic(float *x, const idx_t *groundtruth, size_t gt_dim, size_t qsize)
        {
            for (int k = 0; k < 21; k++) {
                const size_t maxcodes = 1 << k;
                const size_t probes = (k <= 16) ? 128 : 1280;
                quantizer->ef_ = (k <= 16) ? 140 : 1280;

                double correct = 0;
                idx_t keys[probes];
                float q_c[probes];

                for (int q_idx = 0; q_idx < qsize; q_idx++) {
                    auto coarse = quantizer->searchKnn(x + q_idx*d, probes);
                    idx_t gt = groundtruth[gt_dim * q_idx];

                    for (int i = probes - 1; i >= 0; i--) {
                        auto elem = coarse.top();
                        q_c[i] = elem.first;
                        keys[i] = elem.second;
                        coarse.pop();
                    }

                    size_t counter = 0;
                    for (int i = 0; i < probes; i++) {
                        idx_t centroid_num = keys[i];
                        if (group_sizes[centroid_num].size() == 0)
                            continue;

                        const idx_t *id = ids[centroid_num].data();
                        const idx_t *nn_centroids = nn_centroid_idxs[centroid_num].data();
                        float alpha = alphas[centroid_num];

                        const float *centroid = (float *) quantizer->getDataByInternalId(centroid_num);

                        std::vector<float> q_s(nsubc);
                        std::vector<float> r(nsubc);

                        /** Filtering **/

                        std::vector < int > offset(nsubc);
                        std::priority_queue<std::pair<float, idx_t>, std::vector<std::pair<float, idx_t>>, CompareByFirst> ordered_subc;
                        for (int subc = 0; subc < nsubc; subc++) {
                            idx_t subcentroid_num = nn_centroids[subc];
                            const float *nn_centroid = (float *) quantizer->getDataByInternalId(subcentroid_num);

                            q_s[subc] = faiss::fvec_L2sqr(x+q_idx*d, nn_centroid, d);
                            r[subc] = (1-alpha) * q_c[i] + alpha * (alpha-1) * s_c[centroid_num][subc] + alpha * q_s[subc];

                            ordered_subc.emplace(std::make_pair(-r[subc], subc));

                            if (subc == 0)
                                offset[subc] = 0;
                            else
                                offset[subc] = offset[subc-1] + group_sizes[centroid_num][subc-1];
                        }

//                        double max_r = 0.0;
//                        double average_r = 0.0;
//                        for (int subc = 0; subc < nsubc; subc++) {
//                            idx_t subcentroid_num = nn_centroids[subc];
//                            const float *nn_centroid = (float *) quantizer->getDataByInternalId(subcentroid_num);
//
//                            q_s[subc] = faiss::fvec_L2sqr(x+q_idx*d, nn_centroid, d);
//                            r[subc] = (1 - alpha) * q_c[i] + alpha * (alpha - 1) * s_c[centroid_num][subc] +
//                                      alpha * q_s[subc];
//
//                            if (r[subc] > max_r)
//                                max_r = r[subc];
//
//                            average_r += r[subc];
//                        }
//                        average_r /= nsubc;

                        int prev_correct = correct;
                        while (ordered_subc.size() >= nsubc/2){
                            idx_t subc = ordered_subc.top().second;
                            ordered_subc.pop();
                        //for (int subc = 0; subc < nsubc; subc++) {
                            int groupsize = group_sizes[centroid_num][subc];
                            if (groupsize == 0)
                                continue;

//                            if (r[subc] > (average_r+max_r)/2) {
//                                id += groupsize;
//                                continue;
//                            }

                            for (int j = 0; j < groupsize; j++) {
                                if (id[offset[subc] + j] == gt) {
                                    correct++;
                                    break;
                                }
                                counter++;
                                if (counter == maxcodes)
                                    break;
                            }
                            if (counter == maxcodes || correct == prev_correct + 1)
                                break;
                            /** Shift to the next group **/
                            //id += groupsize;
                        }
                        if (counter == maxcodes || correct == prev_correct + 1)
                            break;
                    }
                }
                std::cout << k << " " << correct / qsize << std::endl;
            }
            std::cout << "HUI" << std::endl;
        }

	private:
        std::vector<float> dis_table;
        std::vector<float> norms;
        std::vector<float> centroid_norms;

        float fstdistfunc(uint8_t *code)
        {
            float result = 0.;
            int dim = code_size >> 2;
            int m = 0;
            for (int i = 0; i < dim; i++) {
                result += dis_table[pq->ksub * m + code[m]]; m++;
                result += dis_table[pq->ksub * m + code[m]]; m++;
                result += dis_table[pq->ksub * m + code[m]]; m++;
                result += dis_table[pq->ksub * m + code[m]]; m++;
            }
            return result;
        }
    public:
        void compute_residuals(size_t n, float *residuals, const float *points, const float *subcentroids, const idx_t *keys)
		{
            //#pragma omp parallel for num_threads(16)
            for (idx_t i = 0; i < n; i++) {
                const float *subcentroid = subcentroids + keys[i]*d;
                const float *point = points + i*d;
                for (int j = 0; j < d; j++) {
                    residuals[i*d + j] = point[j] - subcentroid[j];
                }
            }
		}

        void reconstruct(size_t n, float *x, const float *decoded_residuals, const float *subcentroids, const idx_t *keys)
        {
            //#pragma omp parallel for num_threads(16)
            for (idx_t i = 0; i < n; i++) {
                const float *subcentroid = subcentroids + keys[i]*d;
                const float *decoded_residual = decoded_residuals + i*d;
                for (int j = 0; j < d; j++)
                    x[i*d + j] = subcentroid[j] + decoded_residual[j];
            }
        }

        /** NEW **/
        void sub_vectors(float *target, const float *x, const float *y)
        {
            for (int i = 0; i < d; i++)
                target[i] = x[i] - y[i];
        }

        void compute_subcentroid_idxs(idx_t *subcentroid_idxs, const float *subcentroids,
                                      const float *points, const int groupsize)
        {
            //#pragma omp parallel for num_threads(16)
            for (int i = 0; i < groupsize; i++) {
                std::priority_queue<std::pair<float, idx_t>> max_heap;
                for (int subc = 0; subc < nsubc; subc++) {
                    const float *subcentroid = subcentroids + subc * d;
                    const float *point = points + i * d;
                    float dist = faiss::fvec_L2sqr(subcentroid, point, d);
                    max_heap.emplace(std::make_pair(-dist, subc));
                }
                subcentroid_idxs[i] = max_heap.top().second;
            }

        }

        void compute_vectors(float *target, const float *x, const float *centroid, const int n)
        {
            //#pragma omp parallel for num_threads(16)
            for (int i = 0; i < n; i++)
                for (int j = 0; j < d; j++)
                    target[i*d + j] = x[i*d + j] - centroid[j];
        }

        float compute_alpha(const float *centroid_vectors, const float *points,
                            const float *centroid, const float *centroid_vector_norms_L2sqr,
                            const int groupsize)
        {
            int counter_positive = 0;
            int counter_negative = 0;
            float positive_alpha = 0.0;
            float negative_alpha = 0.0;

            std::vector<float> point_vectors(groupsize*d);
            compute_vectors(point_vectors.data(), points, centroid, groupsize);

            for (int i = 0; i < groupsize; i++) {
                const float *point_vector = point_vectors.data() + i * d;
                const float *point = points + i * d;

                std::priority_queue<std::pair<float, float>> max_heap;
                for (int subc = 0; subc < nsubc; subc++){
                    const float *centroid_vector = centroid_vectors + subc * d;
                    const float centroid_vector_norm_L2sqr = centroid_vector_norms_L2sqr[subc];

                    float alpha = faiss::fvec_inner_product (centroid_vector, point_vector, d);
                    alpha /= centroid_vector_norm_L2sqr;

                    std::vector<float> subcentroid(d);
                    faiss::fvec_madd (d, centroid, alpha, centroid_vector, subcentroid.data());

                    float dist = faiss::fvec_L2sqr(point, subcentroid.data(), d);
                    max_heap.emplace(std::make_pair(-dist, alpha));
                }
                float optim_alpha = max_heap.top().second;
                if (optim_alpha < 0) {
                    counter_negative++;
                    negative_alpha += optim_alpha;
                } else {
                    counter_positive++;
                    positive_alpha += optim_alpha;
                }
            }
            positive_alpha /= counter_positive;
            negative_alpha /= counter_negative;
            return (counter_positive > counter_negative) ? positive_alpha : negative_alpha;
        }

        struct CompareByFirst {
            constexpr bool operator()(std::pair<float, idx_t> const &a,
                                      std::pair<float, idx_t> const &b) const noexcept {
                return a.first < b.first;
            }
        };
	};

}
