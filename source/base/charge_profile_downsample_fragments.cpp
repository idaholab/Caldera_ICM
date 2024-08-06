
#include "charge_profile_downsample_fragments.h"

#include <cmath>            // max, round, pow
#include <algorithm>        // sort
#include <iterator>
#include <unordered_set>


downsample_charge_fragment_vector::downsample_charge_fragment_vector( const pev_charge_fragment_removal_criteria fragment_removal_criteria_ )
{
    this->fragment_removal_criteria = fragment_removal_criteria_;
    this->change_metric_multiplier = std::pow(10, 12);  // 10^12

    // std::cout << "    max_percent_of_fragments_that_can_be_removed: " << this->fragment_removal_criteria.max_percent_of_fragments_that_can_be_removed << std::endl;
    // std::cout << "    perc_change_threshold: " << this->fragment_removal_criteria.perc_change_threshold << std::endl;
    // std::cout << "    kW_change_threshold: " << this->fragment_removal_criteria.kW_change_threshold << std::endl;
    // std::cout << "    threshold_to_determine_not_removable_fragments_on_flat_peak_kW: " << this->fragment_removal_criteria.threshold_to_determine_not_removable_fragments_on_flat_peak_kW << std::endl;
    // std::cout << "    perc_of_max_starting_point_to_determine_not_removable_fragments_on_low_elbow: " << this->fragment_removal_criteria.perc_of_max_starting_point_to_determine_not_removable_fragments_on_low_elbow << std::endl;
    // std::cout << std::endl;    
}


std::vector<pev_charge_fragment_variation> downsample_charge_fragment_vector::get_removed_fragments()
{
    return this->removed_fragments;
}


std::vector<pev_charge_fragment_variation> downsample_charge_fragment_vector::get_retained_fragments()
{
    return this->retained_fragments;
}


int64_t downsample_charge_fragment_vector::calculate_variation_rank( const std::list<pev_charge_fragment_variation>::iterator it,
                                                                     std::mt19937& gen,
                                                                     std::uniform_int_distribution<>& dis )
{
    std::list<pev_charge_fragment_variation>::iterator it_next, it_prev;
    
    it_next = std::next(it);
    it_prev = std::prev(it);
    
    double P3kW_delta_A, P3kW_delta_B, P3kW_delta_C, P3kW_delta;
    
    P3kW_delta_A = std::abs(it->P3_kW - it_prev->P3_kW);
    P3kW_delta_B = std::abs(it_next->P3_kW - it->P3_kW);
    P3kW_delta_C = std::abs(it_next->P3_kW - it_prev->P3_kW);    
    P3kW_delta = std::max({P3kW_delta_A, P3kW_delta_B, P3kW_delta_C});
        
    //-----------------------------------------------------------------------------------
    // max int64_t = 9,223,372.036,854,780,000
    //        Random Part -> Smallest 9 digits, [0, 999,999,999]
    //        kW_change_metric -> Next 8 digits given the following format x,xxx.xxx kW
    //        change_metric_multiplier = 9 digit + 3 digits = 10^12
    //-----------------------------------------------------------------------------------
    
    double kW_change_metric = P3kW_delta / this->fragment_removal_criteria.kW_change_threshold;
    int64_t tmp_variation_rank = (int64_t)((kW_change_metric)*this->change_metric_multiplier);
    int64_t variation_rank;
    while(true)
    {
        variation_rank = tmp_variation_rank + dis(gen);
        if(this->variationRank_to_fragmentVariationIt_map.count(variation_rank) == 0)
            break;
    }
    
    return variation_rank;
}


int downsample_charge_fragment_vector::get_index_of_elbow( const int start_index,
                                                           const int end_index,
                                                           const std::list<pev_charge_fragment_variation>& fragment_variations )
{
    std::list<pev_charge_fragment_variation>::const_iterator it;
    
    it = fragment_variations.begin();
    it = std::next(it, start_index);
    double soc_0 = it->soc;
    double P3kW_0 = it->P3_kW;
    
    it = std::next(it, end_index - start_index);
    double soc_1 = it->soc;
    double P3kW_1 = it->P3_kW;
    
    double m = (P3kW_1 - P3kW_0) / (soc_1 - soc_0);
    double b = P3kW_1 - m*soc_1;
    
    //----------------------
    
    it = fragment_variations.begin();
    it = std::next(it, start_index);
        
    double tmp_diff, max_diff_P3kW = 0;
    int index_of_elbow = 0;
    
    for(int i=start_index; i<end_index; i++)
    {
        tmp_diff = std::abs(it->P3_kW - (m*it->soc + b));
        if(tmp_diff > max_diff_P3kW)
        {
            max_diff_P3kW = tmp_diff;
            index_of_elbow = it->original_charge_fragment_index;
        }
        
        it = std::next(it);
    }
    
    return index_of_elbow;
}


void downsample_charge_fragment_vector::downsample( std::vector<pev_charge_fragment>& original_charge_fragments,
                                                    std::vector<pev_charge_fragment>& downsampled_charge_fragments )
{
    downsampled_charge_fragments.clear();
    this->variationRank_to_fragmentVariationIt_map.clear();
    this->charge_fragment_variations.clear();
    this->removed_fragments.clear();
    this->retained_fragments.clear();
    
    std::random_device rd;  
    std::mt19937 gen(rd()); 
    std::uniform_int_distribution<> dis(0, 999999999);
    
    //---------------------------------------------
    //   Create pev_charge_fragment_variation List
    //---------------------------------------------

    std::sort(original_charge_fragments.begin(), original_charge_fragments.end());
    int num_original_fragments = original_charge_fragments.size();
    
    const pev_charge_fragment* fragment_ptr;
    double P3_kW;    
    int i = 0;

    fragment_ptr = &original_charge_fragments.at(i);
    
    if(std::abs(fragment_ptr->time_since_charge_began_hrs) < 0.000001)
    {
        P3_kW = 0;
        this->charge_fragment_variations.emplace_back(i, fragment_ptr->time_since_charge_began_hrs, fragment_ptr->soc, P3_kW);
    }
    else
    {
        P3_kW = fragment_ptr->E3_kWh / fragment_ptr->time_since_charge_began_hrs;
        this->charge_fragment_variations.emplace_back(i, fragment_ptr->time_since_charge_began_hrs, fragment_ptr->soc, P3_kW);
    }

    for(i=1; i<num_original_fragments; i++)
    {
        fragment_ptr = &original_charge_fragments.at(i);
        P3_kW = (original_charge_fragments.at(i).E3_kWh - original_charge_fragments.at(i-1).E3_kWh) / (original_charge_fragments.at(i).time_since_charge_began_hrs - original_charge_fragments.at(i-1).time_since_charge_began_hrs);
        this->charge_fragment_variations.emplace_back(i, fragment_ptr->time_since_charge_began_hrs, fragment_ptr->soc, P3_kW);
    }
    
    //----------------------------------------------------------------------
    //             Create  unremovable_charge_fragment_indexes
    //----------------------------------------------------------------------
    
    std::unordered_set<int> unremovable_charge_fragment_indexes = {0, num_original_fragments-1};
    
    //----------------------------------------------------------------------
    //   Identify Charge Fragments on 'flat peak' that are not 'Removable'
    //----------------------------------------------------------------------
   
    std::list<pev_charge_fragment_variation>::iterator it;
    
    std::vector<pev_charge_fragment_variation> fragments_with_max_P3_kW;
    double max_P3_kW = -1;
   
    for(it=this->charge_fragment_variations.begin(); it!=this->charge_fragment_variations.end(); it++)
        if(it->P3_kW > max_P3_kW)
            max_P3_kW = it->P3_kW;

    for(it=this->charge_fragment_variations.begin(); it!=this->charge_fragment_variations.end(); it++)
        if(std::abs(max_P3_kW - it->P3_kW) < this->fragment_removal_criteria.threshold_to_determine_not_removable_fragments_on_flat_peak_kW)
            fragments_with_max_P3_kW.push_back(*it);
    
    int not_removable_fragment_index_flat_LB = fragments_with_max_P3_kW[0].original_charge_fragment_index;
    int not_removable_fragment_index_flat_UB = fragments_with_max_P3_kW[fragments_with_max_P3_kW.size()-1].original_charge_fragment_index;
    
    if(unremovable_charge_fragment_indexes.count(not_removable_fragment_index_flat_LB) == 0)
        unremovable_charge_fragment_indexes.insert(not_removable_fragment_index_flat_LB);
    
    if(unremovable_charge_fragment_indexes.count(not_removable_fragment_index_flat_UB) == 0)
        unremovable_charge_fragment_indexes.insert(not_removable_fragment_index_flat_UB);
    
    //----------------------------------------------------------------------
    //   Identify Charge Fragments on 'low elbow' that are not 'Removable'
    //----------------------------------------------------------------------

    if(0 < this->fragment_removal_criteria.perc_of_max_starting_point_to_determine_not_removable_fragments_on_low_elbow)
    {
        int index_UB = not_removable_fragment_index_flat_LB;
        
        //------------------------
        //    Find  index_LB
        //------------------------
        int index_LB = -1;
        double start_P3_kW = max_P3_kW * this->fragment_removal_criteria.perc_of_max_starting_point_to_determine_not_removable_fragments_on_low_elbow/100.0;
        
        for(it=this->charge_fragment_variations.begin(); it!=this->charge_fragment_variations.end(); it++)
        {
            if(it->P3_kW > start_P3_kW)
            {
                index_LB = it->original_charge_fragment_index;
                break;
            }
        }
        
        //------------------------
        //    Find  Elbows
        //------------------------
        int elbow_center = get_index_of_elbow(index_LB, index_UB, this->charge_fragment_variations);
        int elbow_lower = get_index_of_elbow(index_LB, elbow_center, this->charge_fragment_variations);
        int elbow_upper = get_index_of_elbow(elbow_center, index_UB, this->charge_fragment_variations);
        
        if(unremovable_charge_fragment_indexes.count(elbow_center) == 0)
            unremovable_charge_fragment_indexes.insert(elbow_center);
        
        if(unremovable_charge_fragment_indexes.count(elbow_lower) == 0)
            unremovable_charge_fragment_indexes.insert(elbow_lower);
        
        if(unremovable_charge_fragment_indexes.count(elbow_upper) == 0)
            unremovable_charge_fragment_indexes.insert(elbow_upper);
    }
    
    //------------------------------------------------------------------
    //    Calculate:   variation_rank
    //     Populate:   this->variationRank_to_fragmentVariationIt_map
    //------------------------------------------------------------------

    int64_t variation_rank;

    for(it=this->charge_fragment_variations.begin(); it!=this->charge_fragment_variations.end(); it++)
    {
        if(unremovable_charge_fragment_indexes.count(it->original_charge_fragment_index) == 0)
        {
            it->is_removable = true;
            variation_rank = calculate_variation_rank(it, gen, dis);                //  Calculate variation_rank
            it->variation_rank = variation_rank;                                    //  Update List with variation_rank
            this->variationRank_to_fragmentVariationIt_map[variation_rank] = it;    //  Update Map with variation_rank
        }
        else
            it->is_removable = false;
    }
    

    //------------------------------------------------------------
    //             Determine Indexes to Remove
    //------------------------------------------------------------
    
    std::list<pev_charge_fragment_variation>::iterator it_prev, it_next;
    std::unordered_set<int> indexes_to_remove;
        
    int max_number_of_fragments_that_can_be_removed = (int)num_original_fragments*this->fragment_removal_criteria.max_percent_of_fragments_that_can_be_removed/100;
    int64_t min_variation_rank;    

    while(true)
    {
        min_variation_rank = this->variationRank_to_fragmentVariationIt_map.begin()->first;
        
        if(min_variation_rank > (int64_t)this->change_metric_multiplier)
            break;
        
        //--------------------
        
        it = this->variationRank_to_fragmentVariationIt_map[min_variation_rank];
        it_prev = std::prev(it);
        it_next = std::next(it);
        
        //--------------------------------------
        //    Remove Current List Element
        //--------------------------------------
        indexes_to_remove.insert(it->original_charge_fragment_index);                   //  Log Charge Fragment Index to Remove
        removed_fragments.push_back(*it);                                               //  Bookeeping (Saving removed fragments)
        this->variationRank_to_fragmentVariationIt_map.erase(min_variation_rank);       //  Erase Fragment from Map
        this->charge_fragment_variations.erase(it);                                     //  Erase Fragment from List 
        
        //--------------------------------------
        //    Update Previous List Element
        //--------------------------------------
        if(it_prev->is_removable)
        {
            this->variationRank_to_fragmentVariationIt_map.erase(it_prev->variation_rank);  //  Erase from Map Old variation_rank
            variation_rank = calculate_variation_rank(it_prev, gen, dis);                   //  Recalculating variation_rank
            it_prev->variation_rank = variation_rank;                                       //  Update List with new variation_rank
            this->variationRank_to_fragmentVariationIt_map[variation_rank] = it_prev;       //  Update map with new variation_rank 
        }
            
        //--------------------------------------
        //    Update Next List Element
        //--------------------------------------
        if(it_next->is_removable)
        {
            this->variationRank_to_fragmentVariationIt_map.erase(it_next->variation_rank);  //  Erase Pevious variation_rank from Map
            variation_rank = calculate_variation_rank(it_next, gen, dis);                   //  Recalculating variation_rank
            it_next->variation_rank = variation_rank;                                       //  Update List with new variation_rank
            this->variationRank_to_fragmentVariationIt_map[variation_rank] = it_next;       //  Update map with new variation_rank 
        }
        
        //--------------------
        
        if(indexes_to_remove.size() >= max_number_of_fragments_that_can_be_removed)
            break;
    }
   
    //------------------------------------
    
    for(it=this->charge_fragment_variations.begin(); it != this->charge_fragment_variations.end(); it++)
        this->retained_fragments.push_back(*it);
    
    //------------------------------------------------------------
    //               Downsample Charge Fragments
    //------------------------------------------------------------

    for(int i=0; i<num_original_fragments; i++)
    {
        if(indexes_to_remove.count(i) == 0)
            downsampled_charge_fragments.push_back(original_charge_fragments.at(i));
    }
}

