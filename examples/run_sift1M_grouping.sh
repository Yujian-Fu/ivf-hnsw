#!/bin/bash

################################
# HNSW construction parameters #
################################

M="16"                # Min number of edges per point
efConstruction="500"  # Max number of candidate vertices in priority queue to observe during construction

###################
# Data parameters #
###################

nb="1000000"       # Number of base vectors

nt="100000"         # Number of learn vectors
nsubt="100000"         # Number of learn vectors to train (random subset of the learn set)

nc="10000"           # Number of centroids for HNSW quantizer
nsubc="64"            # Number of subcentroids per group

nq="1000"            # Number of queries
ngt="100"            # Number of groundtruth neighbours per query

d="128"               # Vector dimension

#################
# PQ parameters #
#################

code_size="16"        # Code size per vector in bytes
opq="off"             # Turn on/off opq encoding

#####################
# Search parameters #
#####################

#######################################
#        Paper configurations         #
# (<nprobe>, <max_codes>, <efSearch>) #
# (   32,       10000,        80    ) #
# (   64,       30000,       100    ) #
# (  128,      100000,       130    ) #
#######################################

k="10"                 # Number of the closest vertices to search
nprobe="100"           # Number of probes at query time
max_codes="3000"     # Max number of codes to visit to do a query
efSearch="100"        # Max number of candidate vertices in priority queue to observe during searching
pruning = "off"

#########
# Paths #
#########

path_data="${PWD}/data/SIFT1M"
path_model="${PWD}/models/SIFT1M"

path_base="${path_data}/sift_base.fvecs"
path_learn="${path_data}/sift_learn.fvecs"
path_gt="${path_data}/sift_groundtruth.ivecs"
path_q="${path_data}/sift_query.fvecs"
path_centroids="${path_data}/centroids_sift1M.fvecs"

path_precomputed_idxs="${path_data}/precomputed_idxs_sift1M.ivecs"

path_edges="${path_model}/hnsw_M${M}_ef${efConstruction}.ivecs"
path_info="${path_model}/hnsw_M${M}_ef${efConstruction}.bin"

path_pq="${path_model}/pq${code_size}_nsubc${nsubc}.pq"
path_norm_pq="${path_model}/norm_pq${code_size}_nsubc${nsubc}.pq"
path_index="${path_model}/ivfhnsw_PQ${code_size}_nsubc${nsubc}.index"

#######
# Run #
#######
${PWD}/bin/test_ivfhnsw_grouping_sift1M \
                                -M ${M} \
                                -efConstruction ${efConstruction} \
                                -nb ${nb} \
                                -nt ${nt} \
                                -nsubt ${nsubt} \
                                -nc ${nc} \
                                -nsubc ${nsubc} \
                                -nq ${nq} \
                                -ngt ${ngt} \
                                -d ${d} \
                                -code_size ${code_size} \
                                -opq ${opq} \
                                -k ${k} \
                                -nprobe ${nprobe} \
                                -max_codes ${max_codes} \
                                -efSearch ${efSearch} \
                                -path_base ${path_base} \
                                -path_learn ${path_learn} \
                                -path_gt ${path_gt} \
                                -path_q ${path_q} \
                                -path_centroids ${path_centroids} \
                                -path_precomputed_idx ${path_precomputed_idxs} \
                                -path_edges ${path_edges} \
                                -path_info ${path_info} \
                                -path_pq ${path_pq} \
                                -path_norm_pq ${path_norm_pq} \
                                -path_index ${path_index} \
                                -pruning ${pruning}