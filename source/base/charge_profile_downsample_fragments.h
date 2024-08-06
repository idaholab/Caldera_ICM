
#ifndef inl_pev_charge_profile_factory_H
#define inl_pev_charge_profile_factory_H

#include "datatypes_global.h"   // pev_charge_fragment_removal_criteria

#include <vector>
#include <map>
#include <list>
#include <cstdint>          // int64_t
#include <random>           // Uniform random number generator

//====================================


class downsample_charge_fragment_vector
{
private:
    pev_charge_fragment_removal_criteria fragment_removal_criteria;
    double change_metric_multiplier;
    
    std::list<pev_charge_fragment_variation> charge_fragment_variations;
    std::map<int64_t, std::list<pev_charge_fragment_variation>::iterator> variationRank_to_fragmentVariationIt_map;

    std::vector<pev_charge_fragment_variation> removed_fragments;
    std::vector<pev_charge_fragment_variation> retained_fragments;
    
    int64_t calculate_variation_rank( const std::list<pev_charge_fragment_variation>::iterator it,
                                      std::mt19937& gen,
                                      std::uniform_int_distribution<>& dis );
    int get_index_of_elbow( const int start_index,
                            const int end_index,
                            const std::list<pev_charge_fragment_variation>& fragment_variations );
    
public:
    downsample_charge_fragment_vector() {};
    downsample_charge_fragment_vector( const pev_charge_fragment_removal_criteria fragment_removal_criteria_ );
    
    std::vector<pev_charge_fragment_variation> get_removed_fragments();
    std::vector<pev_charge_fragment_variation> get_retained_fragments();

    void downsample( std::vector<pev_charge_fragment>& original_charge_fragments,
                     std::vector<pev_charge_fragment>& downsampled_charge_fragments );
};


#endif

