/******************************************************************************
 * kaffpa_interface.cpp 
 *
 * Source of KaHIP -- Karlsruhe High Quality Partitioning.
 *
 ******************************************************************************
 * Copyright (C) 2013-2015 Christian Schulz <christian.schulz@kit.edu>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *****************************************************************************/

#include <iostream>
#include <sstream>

#ifdef USEMETIS
        #include "metis.h"
#endif

#include "kaHIP_interface.h"
#include "../lib/data_structure/graph_access.h"
#include "../lib/io/graph_io.h"
#include "../lib/node_ordering/nested_dissection.h"
#include "../lib/tools/timer.h"
#include "../lib/tools/quality_metrics.h"
#include "../lib/tools/macros_assertions.h"
#include "../lib/tools/random_functions.h"
//#include "../lib/parallel_mh/parallel_mh_async.h"
#include "../lib/partition/uncoarsening/separator/area_bfs.h"
#include "../lib/partition/partition_config.h"
#include "../lib/partition/graph_partitioner.h"
#include "../lib/partition/uncoarsening/separator/vertex_separator_algorithm.h"
#include "../app/configuration.h"
#include "../app/balance_configuration.h"

using namespace std;

void internal_build_graph( PartitionConfig & partition_config, 
                           int* n, 
                           int* vwgt, 
                           int* xadj, 
                           int* adjcwgt, 
                           int* adjncy,
                           graph_access & G) {
        G.build_from_metis(*n, xadj, adjncy); 
        G.set_partition_count(partition_config.k); 
 
        srand(partition_config.seed);
        random_functions::setSeed(partition_config.seed);
       
        if(vwgt != NULL) {
                forall_nodes(G, node) {
                        G.setNodeWeight(node, vwgt[node]);
                } endfor
        }

        if(adjcwgt != NULL) {
                forall_edges(G, e) {
                        G.setEdgeWeight(e, adjcwgt[e]);
                } endfor 
        }

        balance_configuration bc;
        bc.configurate_balance( partition_config, G);
}

void internal_kaffpa_call(PartitionConfig & partition_config, 
                          bool suppress_output, 
                          int* n, 
                          int* vwgt, 
                          int* xadj, 
                          int* adjcwgt, 
                          int* adjncy, 
                          int* nparts, 
                          double* imbalance, 
                          int* edgecut, 
                          int* part) {

        //streambuf* backup = cout.rdbuf();
        //ofstream ofs;
        //ofs.open("/dev/null");
        //if(suppress_output) {
               //cout.rdbuf(ofs.rdbuf()); 
        //}

        partition_config.imbalance = 100*(*imbalance);
        graph_access G;     
        internal_build_graph( partition_config, n, vwgt, xadj, adjcwgt, adjncy, G);

        graph_partitioner partitioner;
        partitioner.perform_partitioning(partition_config, G);

        forall_nodes(G, node) {
                part[node] = G.getPartitionIndex(node);
        } endfor

        quality_metrics qm;
        *edgecut = qm.edge_cut(G);

        //ofs.close();
        //cout.rdbuf(backup);
}

void kaffpa(int* n, 
                   int* vwgt, 
                   int* xadj, 
                   int* adjcwgt, 
                   int* adjncy, 
                   int* nparts, 
                   double* imbalance, 
                   bool suppress_output, 
                   int seed,
                   int mode,
                   int* edgecut, 
                   int* part) {
        configuration cfg;
        PartitionConfig partition_config;
        partition_config.k = *nparts;

        switch( mode ) {
                case FAST: 
                        cfg.fast(partition_config);
                        break;
                case ECO: 
                        cfg.eco(partition_config);
                        break;
                case STRONG: 
                        cfg.strong(partition_config);
                        break;
                case FASTSOCIAL: 
                        cfg.fastsocial(partition_config);
                        break;
                case ECOSOCIAL: 
                        cfg.ecosocial(partition_config);
                        break;
                case STRONGSOCIAL: 
                        cfg.strongsocial(partition_config);
                        break;
                default: 
                        cfg.eco(partition_config);
                        break;
        }

        partition_config.seed = seed;
        internal_kaffpa_call(partition_config, suppress_output, n, vwgt, xadj, adjcwgt, adjncy, nparts, imbalance, edgecut, part);
}

void kaffpa_balance_NE(int* n, 
                   int* vwgt, 
                   int* xadj, 
                   int* adjcwgt, 
                   int* adjncy, 
                   int* nparts, 
                   double* imbalance, 
                   bool suppress_output, 
                   int seed,
                   int mode,
                   int* edgecut, 
                   int* part) {
        configuration cfg;
        PartitionConfig partition_config;
        partition_config.k = *nparts;

        switch( mode ) {
                case FAST: 
                        cfg.fast(partition_config);
                        break;
                case ECO: 
                        cfg.eco(partition_config);
                        break;
                case STRONG: 
                        cfg.strong(partition_config);
                        break;
                case FASTSOCIAL: 
                        cfg.fastsocial(partition_config);
                        break;
                case ECOSOCIAL: 
                        cfg.ecosocial(partition_config);
                        break;
                case STRONGSOCIAL: 
                        cfg.strongsocial(partition_config);
                        break;
                default: 
                        cfg.eco(partition_config);
                        break;
        }

        partition_config.seed = seed;
        partition_config.balance_edges = true;
        internal_kaffpa_call(partition_config, suppress_output, n, vwgt, xadj, adjcwgt, adjncy, nparts, imbalance, edgecut, part);
}

void internal_nodeseparator_call(PartitionConfig & partition_config, 
                          bool suppress_output, 
                          int* n, 
                          int* vwgt, 
                          int* xadj, 
                          int* adjcwgt, 
                          int* adjncy, 
                          int* nparts, 
                          double* imbalance, 
                          int mode,
                          int* num_nodeseparator_vertices, 
                          int** separator) {

        //first perform std partitioning using KaFFPa
        streambuf* backup = cout.rdbuf();
        ofstream ofs;
        ofs.open("/dev/null");
        if(suppress_output) {
               cout.rdbuf(ofs.rdbuf()); 
        }

        partition_config.k         = *nparts;
        partition_config.imbalance = 100*(*imbalance);
        graph_access G;     
        internal_build_graph( partition_config, n, vwgt, xadj, adjcwgt, adjncy, G);
        graph_partitioner partitioner;

        area_bfs::m_deepth.resize(G.number_of_nodes());
        forall_nodes(G, node) {
                area_bfs::m_deepth[node] = 0;
        } endfor

        if( partition_config.k > 2 ) {
                partitioner.perform_partitioning(partition_config, G);

                // now compute a node separator from the partition of the graph
                complete_boundary boundary(&G);
                boundary.build();

                vertex_separator_algorithm vsa;
                std::vector<NodeID> internal_separator;
                vsa.compute_vertex_separator(partition_config, G, boundary, internal_separator);

                // copy to output variables
                *num_nodeseparator_vertices =  internal_separator.size();
                *separator = new int[*num_nodeseparator_vertices];
                for( unsigned int i = 0; i < internal_separator.size(); i++) {
                        (*separator)[i] = internal_separator[i];
                }
        } else {
                
                configuration cfg;
                switch( mode ) {
                        case FAST: 
                                cfg.fast_separator(partition_config);
                                break;
                        case ECO: 
                                cfg.eco_separator(partition_config);
                                break;
                        case STRONG: 
                                cfg.strong_separator(partition_config);
                                break;
                        case FASTSOCIAL: 
                                cfg.fast_separator(partition_config);
                                //cfg.fastsocial_separator(partition_config);
                                break;
                        case ECOSOCIAL: 
                                cfg.eco_separator(partition_config);
                                //cfg.ecosocial_separator(partition_config);
                                break;
                        case STRONGSOCIAL: 
                                //cfg.strongsocial_separator(partition_config);
                                cfg.strong_separator(partition_config);
                                break;
                        default: 
                                cfg.strong_separator(partition_config);
                                break;
                }       
                partition_config.mode_node_separators = true;
                partitioner.perform_partitioning(partition_config, G);
                NodeWeight ns_size = 0;
                forall_nodes(G, node) {
                        if(G.getPartitionIndex(node) == G.getSeparatorBlock()) {
                                ns_size++;
                        }
                } endfor
                *num_nodeseparator_vertices = ns_size;
                *separator = new int[*num_nodeseparator_vertices];
                unsigned int i = 0;
                forall_nodes(G, node) {
                        if(G.getPartitionIndex(node) == G.getSeparatorBlock()) {
                                (*separator)[i] = node;
                                i++;
                        }
                } endfor
        }

        ofs.close();
        cout.rdbuf(backup);
}


void node_separator(int* n, 
                    int* vwgt, 
                    int* xadj, 
                    int* adjcwgt, 
                    int* adjncy, 
                    int* nparts, 
                    double* imbalance, 
                    bool suppress_output, 
                    int seed,
                    int mode,
                    int* num_separator_vertices, 
                    int** separator) {
        configuration cfg;
        PartitionConfig partition_config;
        partition_config.k = *nparts;

        switch( mode ) {
                case FAST: 
                        cfg.fast(partition_config);
                        break;
                case ECO: 
                        cfg.eco(partition_config);
                        break;
                case STRONG: 
                        cfg.strong(partition_config);
                        break;
                case FASTSOCIAL: 
                        cfg.fastsocial(partition_config);
                        break;
                case ECOSOCIAL: 
                        cfg.ecosocial(partition_config);
                        break;
                case STRONGSOCIAL: 
                        cfg.strongsocial(partition_config);
                        break;
                default: 
                        cfg.eco(partition_config);
                        break;
        }
        partition_config.seed = seed;

        internal_nodeseparator_call(partition_config, suppress_output, n, vwgt, xadj, adjcwgt, adjncy, nparts, imbalance, mode, num_separator_vertices, separator);
}

bool internal_parse_reduction_order(const std::string &&order, PartitionConfig &partition_config) {
        std::istringstream stream(order);
        while (!stream.eof()) {
                int value;
                stream >> value;
                if (value >= 0 && value < nested_dissection_reduction_type::num_types) {
                        partition_config.reduction_order.push_back((nested_dissection_reduction_type)value);
                } else {
                        std::cout << "Unknown reduction type " << value << std::endl;
                        return false;
                }
        }
        if (partition_config.reduction_order.empty()) {
                partition_config.disable_reductions = true;
        }
        return true;
}

void reduced_nd(int* n,
                int* vwgt,
                int* xadj,
                int* adjcwgt,
                int* adjncy,
                bool suppress_output,
                int seed,
                int mode,
                double imbalance,
                int rec_limit,
                const char* reduction_order,
                double convergence,
                int max_sim_deg,
                int* ordering) {
        std::streambuf* backup = std::cout.rdbuf();
        if(suppress_output) {
                std::cout.rdbuf(nullptr);
        }

        configuration cfg;
        PartitionConfig partition_config;
        partition_config.k = 2;
        partition_config.dissection_rec_limit = rec_limit;
        partition_config.convergence_factor = convergence;
        partition_config.max_simplicial_degree = max_sim_deg;
        partition_config.disable_reductions = false;

        partition_config.seed = seed;
        srand(partition_config.seed);
        random_functions::setSeed(partition_config.seed);

        switch( mode ) {
                case FAST: 
                        cfg.fast(partition_config);
                        break;
                case ECO: 
                        cfg.eco(partition_config);
                        break;
                case STRONG: 
                        cfg.strong(partition_config);
                        break;
                case FASTSOCIAL: 
                        cfg.fastsocial(partition_config);
                        break;
                case ECOSOCIAL: 
                        cfg.ecosocial(partition_config);
                        break;
                case STRONGSOCIAL: 
                        cfg.strongsocial(partition_config);
                        break;
                default: 
                        cfg.eco(partition_config);
                        break;
        }
        partition_config.seed = seed;
        auto parse_success = internal_parse_reduction_order(std::string(reduction_order), partition_config);
        if (!parse_success) {
                return;
        }

        graph_access G;     
        internal_build_graph( partition_config, n, vwgt, xadj, adjcwgt, adjncy, G);
        
        partition_config.imbalance = 100*imbalance;
        balance_configuration bc;
        bc.configurate_balance(partition_config, G);
        
        nested_dissection dissection(&G);
        dissection.perform_nested_dissection(partition_config);

        for (int i = 0; i < *n; ++i) {
                ordering[i] = dissection.ordering()[i];
        }

        // Restore cout output stream
        std::cout.rdbuf(backup);
}

#ifdef USEMETIS
void reduced_nd_metis(int* n,
                      int* vwgt,
                      int* xadj,
                      int* adjcwgt,
                      int* adjncy,
                      bool suppress_output,
                      int seed,
                      const char* reduction_order,
                      int max_sim_deg,
                      int* ordering) {
        std::streambuf* backup = std::cout.rdbuf();
        if(suppress_output) {
                std::cout.rdbuf(nullptr);
        }

        configuration cfg;
        PartitionConfig partition_config;
        partition_config.k = 2;
        partition_config.max_simplicial_degree = max_sim_deg;
        partition_config.disable_reductions = false;

        partition_config.seed = seed;
        srand(partition_config.seed);
        random_functions::setSeed(partition_config.seed);
        partition_config.seed = seed;
        auto parse_success = internal_parse_reduction_order(std::string(reduction_order), partition_config);
        if (!parse_success) {
                return;
        }

        graph_access input_graph;
        internal_build_graph( partition_config, n, vwgt, xadj, adjcwgt, adjncy, input_graph);
        
        // 'active_graph' is the graph to use after reductions have been applied.
        // If no reductions have been applied, 'active_graph' points to 'input_graph'.
        // Otherwise, it points to 'reduction_stack.back()->get_reduced_graph()'.
        graph_access *active_graph;
        std::vector<std::unique_ptr<Reduction>> reduction_stack;
        bool used_reductions = apply_reductions(partition_config, input_graph, reduction_stack);
        if (used_reductions) {
                active_graph = &reduction_stack.back()->get_reduced_graph();
        } else {
                active_graph = &input_graph;
        }

        idx_t num_nodes = active_graph->number_of_nodes();
        // convert the graph into metis-style
        idx_t* m_xadj = new idx_t[active_graph->number_of_nodes()+1];
        forall_nodes((*active_graph), node) {
                m_xadj[node] = active_graph->get_first_edge(node);
        } endfor
        xadj[num_nodes] = (idx_t)active_graph->number_of_edges();
        idx_t* m_adjncy = new idx_t[active_graph->number_of_edges()];
        forall_edges((*active_graph), edge) {
                m_adjncy[edge] = active_graph->getEdgeTarget(edge);
        } endfor

        idx_t* m_perm = new idx_t[active_graph->number_of_nodes()];
        idx_t* m_iperm = new idx_t[active_graph->number_of_nodes()];      // inverse ordering. This is the one we are interested in.
        idx_t* metis_options = new idx_t[METIS_NOPTIONS];

        // Perform nested dissection with Metis
        if (num_nodes > 0) {
                METIS_SetDefaultOptions(metis_options);
                metis_options[METIS_OPTION_SEED] = seed;
                METIS_NodeND(&num_nodes, m_xadj, m_adjncy, nullptr, metis_options, m_perm, m_iperm);
        }

        std::vector<NodeID> reduced_labels(active_graph->number_of_nodes(), 0);
        for (size_t i = 0; i < active_graph->number_of_nodes(); ++i) {
                reduced_labels[i] = m_iperm[i];
        }

        // Map ordering of reduced graph to input graph
        std::vector<NodeID> final_labels;
        if (used_reductions) {
                map_ordering(reduction_stack, reduced_labels, final_labels);
        } else {
                final_labels = reduced_labels;
        }

        for (int i = 0; i < *n; ++i) {
                ordering[i] = final_labels[i];
        }

        // Restore cout output stream
        std::cout.rdbuf(backup);

        // Delete temporary graph
        delete[] m_xadj;
        delete[] m_adjncy;
        delete[] m_perm;
        delete[] m_iperm;
        delete[] metis_options;

}
#endif
