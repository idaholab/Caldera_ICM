
#include "pev_charge_profile_factory.h"

#include <algorithm>
#include <cmath>

//#############################################################################
//                          linear_regression
//#############################################################################


lin_reg_slope_yinter  linear_regression::non_weighted(const std::vector<xy_point> &points)
{
    double sum_x, sum_xx, sum_y, sum_xy, x, y;
    
    sum_x = 0;
    sum_xx = 0;
    sum_y = 0;
    sum_xy = 0;
    
    for(int i=0; i<points.size(); i++)
    {
        x = points[i].x;
        y = points[i].y;
    
        sum_x += x;
        sum_xx += x*x;
        sum_y += y;
        sum_xy += x*y;
    }
    
    //-------------------------------------------------------------------------------------------------
    //            \  n     sum_x  \                          \ sum_xx  -sum_x \           \ sum_y  \
    //  K = AtA = \               \    inv(K) = (1/det(K)) * \                \     Atb = \        \
    //            \sum_x   sum_xx \                          \ -sum_x     n   \           \ sum_xy \
    //-------------------------------------------------------------------------------------------------
    //  u = inv(AtA)Atb = inv(K)Atb
    //  K = AtA
    //  t -> Transpose
    
    double det, invK_11, invK_12, invK_21, invK_22, n;
    
    n = points.size();
    det = n*sum_xx - sum_x*sum_x;
    
    invK_11 = sum_xx/det;
    invK_12 = -sum_x/det;
    invK_21 = invK_12;
    invK_22 = n/det;
    
    //------------------------------
    
    double m, y_intercept;
    
    y_intercept = invK_11*sum_y + invK_12*sum_xy;
    m = invK_21*sum_y + invK_22*sum_xy;
    
    //------------------------------
    
    lin_reg_slope_yinter return_val = {m, y_intercept};
    return return_val;
}


lin_reg_slope_yinter  linear_regression::weighted(const std::vector<xy_point> &points)
{
    double sum_w, sum_wx, sum_wxx, sum_wy, sum_wxy, x, y, w;
    
    sum_w = 0;
    sum_wx = 0;
    sum_wxx = 0;
    sum_wy = 0;
    sum_wxy = 0;
    
    for(int i=0; i<points.size(); i++)
    {
        x = points[i].x;
        y = points[i].y;
        w = points[i].w;
    
        sum_w += w;
        sum_wx += w*x;
        sum_wxx += w*x*x;
        sum_wy += w*y;
        sum_wxy += w*x*y;
    }
    
    //--------------------------------------------------------------------------------------------------------
    //             \ sum_w    sum_wx \                          \ sum_wxx  -sum_wx \            \ sum_wy  \
    //  K = AtCA = \                 \    inv(K) = (1/det(K)) * \                  \     AtCb = \         \
    //             \sum_wx   sum_wxx \                          \ -sum_wx   sum_w  \            \ sum_wxy \
    //--------------------------------------------------------------------------------------------------------    
    //  u = inv(AtCA)AtCb
    //  K = AtCA
    //  t -> Transpose
    
    double det, invK_11, invK_12, invK_21, invK_22;
    
    det = sum_w*sum_wxx - sum_wx*sum_wx;
    
    invK_11 = sum_wxx/det;
    invK_12 = -sum_wx/det;
    invK_21 = invK_12;
    invK_22 = sum_w/det;
    
    //------------------------------
    
    double m, y_intercept;
    
    y_intercept = invK_11*sum_wy + invK_12*sum_wxy;
    m = invK_21*sum_wy + invK_22*sum_wxy;
    
    //------------------------------
    
    lin_reg_slope_yinter return_val = {m, y_intercept};
    return return_val;
}


//#############################################################################
//                        PEV Charge Profile Factory
//#############################################################################

std::ostream& operator<<(std::ostream& out, line_segment& z)
{
	out << z.x_LB << "," << z.a*z.x_LB + z.b << std::endl;
	out << z.x_UB << "," << z.a*z.x_UB + z.b << std::endl;
	return out;
}

    //#############################################
    //              Ppu_vs_soc_curve
    //#############################################

Ppu_vs_soc_curve::Ppu_vs_soc_curve(double C_rate_, const std::vector<Ppu_vs_soc_point> &points_)
{
    C_rate = C_rate_;
    points = points_;
    
    std::sort(points.begin(), points.end());
}

double Ppu_vs_soc_curve::get_current_rate_amps(double bat_capacity_Ah_1C)
{
    return C_rate * bat_capacity_Ah_1C;
}

const std::vector<Ppu_vs_soc_point>& Ppu_vs_soc_curve::get_points()
{
    return points;
}

    //#############################################
    //           create_dcPkW_from_soc
    //#############################################

create_dcPkW_from_soc::create_dcPkW_from_soc(bool is_charging_not_discharging_, const std::vector<Ppu_vs_soc_curve>& curves_)
{
    is_charging_not_discharging = is_charging_not_discharging_;
    curves = curves_;
    std::sort(curves.rbegin(), curves.rend());
}


void create_dcPkW_from_soc::get_L1_or_L2_charge_profile(double bat_energy_kWh, double charge_rate_kW, std::vector<line_segment> &pev_charge_profile)
{
	Ppu_vs_soc_curve _1c_curve = this->curves[this->curves.size() - 2];
	std::vector<Ppu_vs_soc_point> _1c_points = _1c_curve.get_points();
	std::sort(_1c_points.rbegin(), _1c_points.rend());
	
	Ppu_vs_soc_point point_A = _1c_points[0];
	Ppu_vs_soc_point point_B = _1c_points[1];
	
	double soc_A = point_A.soc;
	double soc_B = point_B.soc;
	
	double P_A = point_A.P_pu * bat_energy_kWh;
	double P_B = point_B.P_pu * bat_energy_kWh;
	
	//-----------------------------------
	
	double m = (P_A - P_B) / (soc_A - soc_B);
	double b = P_A - m * soc_A;
	
	double soc_intersection = (charge_rate_kW - b) / m;
	
	//-----------------------------------
	
	pev_charge_profile.clear();
	
	if(soc_intersection < 99.5)
	{
		pev_charge_profile.push_back({0, soc_intersection, 0, charge_rate_kW});
		pev_charge_profile.push_back({soc_intersection, 100., m, b});
	}
	else
		pev_charge_profile.push_back({0, 100, 0, charge_rate_kW});
}


void create_dcPkW_from_soc::get_dcfc_charge_profile(double bat_energy_kWh, double dc_current_limit, double dc_pwr_limit_kW, double bat_capacity_Ah_1C, std::vector<line_segment> &dcPkW_from_soc, std::vector<bat_objfun_constraints> &constraints)
{
    if(is_charging_not_discharging)
        get_charging_dcfc_charge_profile(bat_energy_kWh, dc_current_limit, dc_pwr_limit_kW, bat_capacity_Ah_1C, dcPkW_from_soc, constraints);
    else
        get_discharging_dcfc_charge_profile(bat_energy_kWh, dc_current_limit, dc_pwr_limit_kW, bat_capacity_Ah_1C, dcPkW_from_soc, constraints);
}

                  
void create_dcPkW_from_soc::get_charging_dcfc_charge_profile(double bat_energy_kWh, double dc_current_limit, double dc_pwr_limit_kW, double bat_capacity_Ah_1C, std::vector<line_segment> &dcPkW_from_soc, std::vector<bat_objfun_constraints> &constraints)
{
    //=====================================================
    //    Interpolate pu curves using dc_current_limit
    //=====================================================
    
    std::vector<line_segment> dcPkW_from_soc_input;
    double soc_0, soc_1, soc_tmp, P_0, P_1, w, P_tmp;
    double m, b, m1, b1;
    
    if(dc_current_limit >= this->curves[0].get_current_rate_amps(bat_capacity_Ah_1C))
    {
        std::vector<Ppu_vs_soc_point> points;
        points = this->curves[0].get_points();
                       
        for(int i=0; i<points.size()-1; i++)
        {
            m = (points[i].P_pu - points[i+1].P_pu) / (points[i].soc - points[i+1].soc);
            b = points[i].P_pu - m * points[i].soc;     
            
            line_segment tmp_seg = {points[i].soc, points[i+1].soc, m, b};            
            dcPkW_from_soc_input.push_back(tmp_seg);
        }
    }
    else
    {
        //--------------------------------------    
        //     Select Curves to Interpolate
        //--------------------------------------
        
        Ppu_vs_soc_curve upper_curve, lower_curve;
        
        for(int i=1; i<this->curves.size(); i++)
        {
            if(this->curves[i].get_current_rate_amps(bat_capacity_Ah_1C) < dc_current_limit)
            {
                upper_curve = this->curves[i-1];      // curves ar getting copied
                lower_curve = this->curves[i];
                break;
            }
        }

        //--------------------------------------
        //       Interpolate Curves
        //--------------------------------------
        
        // interpolated_val = weight_factor*high_val  +  (1 - weight_factor)*low_val;
        double weight_factor = (dc_current_limit - lower_curve.get_current_rate_amps(bat_capacity_Ah_1C)) / (upper_curve.get_current_rate_amps(bat_capacity_Ah_1C) - lower_curve.get_current_rate_amps(bat_capacity_Ah_1C));
        
        std::vector<Ppu_vs_soc_point> upper_points = upper_curve.get_points();      // get points return const reference so this copy is alright
        std::vector<Ppu_vs_soc_point> lower_points = lower_curve.get_points();
        
        //----------------- 
        
        int index_upper_curve, index_lower_curve;
        double next_soc_lower_curve, next_soc_upper_curve;
        bool extend_segment;
        
        index_upper_curve = 0;
        index_lower_curve = 0;
        soc_0 = 0;
        P_0 = weight_factor*upper_points[0].P_pu + (1-weight_factor)*lower_points[0].P_pu;
        
        // upper_points and lower_points are points not segments
        while(true)
        {
            next_soc_lower_curve = lower_points[index_lower_curve + 1].soc;
            next_soc_upper_curve = upper_points[index_upper_curve + 1].soc;
            extend_segment = false;
            
            if(std::abs(next_soc_lower_curve - next_soc_upper_curve) < 1)
            {
                if(upper_points[index_upper_curve + 1].point_type == extend) 
                    extend_segment = true;
                
                soc_1 = next_soc_upper_curve;
                P_1 = weight_factor*upper_points[index_upper_curve+1].P_pu + (1-weight_factor)*lower_points[index_lower_curve+1].P_pu;
                index_lower_curve += 1;
                index_upper_curve += 1;
            }
            else if(next_soc_upper_curve < next_soc_lower_curve)
            {
                if(upper_points[index_upper_curve + 1].point_type == extend) 
                    extend_segment = true;
                
                soc_1 = next_soc_upper_curve;
                
                w = (soc_1 - lower_points[index_lower_curve].soc) / (lower_points[index_lower_curve+1].soc - lower_points[index_lower_curve].soc);
                P_tmp = w*lower_points[index_lower_curve+1].P_pu + (1-w)*lower_points[index_lower_curve].P_pu;
                    
                P_1 = weight_factor*upper_points[index_upper_curve+1].P_pu + (1-weight_factor)*P_tmp;
                index_upper_curve += 1;
            }
            else if(next_soc_lower_curve < next_soc_upper_curve)
            {
                soc_1 = next_soc_lower_curve;
                
                w = (soc_1 - upper_points[index_upper_curve].soc) / (upper_points[index_upper_curve+1].soc - upper_points[index_upper_curve].soc);
                P_tmp = w*upper_points[index_upper_curve+1].P_pu + (1-w)*upper_points[index_upper_curve].P_pu;
                    
                P_1 = weight_factor*P_tmp + (1-weight_factor)*lower_points[index_lower_curve+1].P_pu;
                index_lower_curve += 1;
            }
            
            //-------------------------------
            
            m = (P_1 - P_0) / (soc_1 - soc_0);
            b = P_1 - m*soc_1;
            
            if(extend_segment)
            {
                m1 = (upper_points[index_upper_curve+1].P_pu - upper_points[index_upper_curve].P_pu) / (upper_points[index_upper_curve+1].soc - upper_points[index_upper_curve].soc); 
                b1 = upper_points[index_upper_curve].P_pu - m1*upper_points[index_upper_curve].soc;
                                                 
                soc_1 = (b-b1) / (m1-m);
                P_1 = m*soc_1 + b;
            }
            
            line_segment tmp_seg = {soc_0, soc_1, m, b};
            dcPkW_from_soc_input.push_back(tmp_seg);
            
            soc_0 = soc_1;
            P_0 = P_1;
            
            if(extend_segment)
                break;
        }
        
         //--------------------------------------    
        //       Use Upper Curve Directly
        //--------------------------------------
        
        for(int i=index_upper_curve+1; i<upper_points.size(); i++)
        {
            soc_1 = upper_points[i].soc;
            P_1 = upper_points[i].P_pu;
            
            m = (P_1 - P_0) / (soc_1 - soc_0);
            b = P_1 - m*soc_1;
            
            line_segment tmp_seg = {soc_0, soc_1, m, b};
            dcPkW_from_soc_input.push_back(tmp_seg);
                                          
            soc_0 = soc_1;
            P_0 = P_1;
        }
    }
    
    //=====================================================
    //            Scale Using Battery Energy
    //=====================================================
    
    for(int i=0; i<dcPkW_from_soc_input.size(); i++)
    {
        dcPkW_from_soc_input[i].a = dcPkW_from_soc_input[i].a * bat_energy_kWh;
        dcPkW_from_soc_input[i].b = dcPkW_from_soc_input[i].b * bat_energy_kWh;
    }
    
    //=====================================================
    //                Apply Power Limits
    //=====================================================
    
    for(int i=0; i<dcPkW_from_soc_input.size(); i++)
    {
        soc_0 = dcPkW_from_soc_input[i].x_LB;
        soc_1 = dcPkW_from_soc_input[i].x_UB;
        m = dcPkW_from_soc_input[i].a;
        b = dcPkW_from_soc_input[i].b;
    
        P_0 = m*soc_0 + b;
        P_1 = m*soc_1 + b;
        
        if(dc_pwr_limit_kW <= P_0 && dc_pwr_limit_kW <= P_1)
        {
            dcPkW_from_soc_input[i].a = 0;
            dcPkW_from_soc_input[i].b = dc_pwr_limit_kW;        
        }
        else if(P_0 < dc_pwr_limit_kW && dc_pwr_limit_kW < P_1)
        {
            soc_tmp = (dc_pwr_limit_kW - b)/m;
            dcPkW_from_soc_input[i].x_UB = soc_tmp;
            
            line_segment tmp_seg = {soc_tmp, soc_1, 0, dc_pwr_limit_kW};
            dcPkW_from_soc_input.push_back(tmp_seg);
        }
        else if(dc_pwr_limit_kW < P_0 && P_1 < dc_pwr_limit_kW)
        {
            soc_tmp = (dc_pwr_limit_kW - b)/m;
            dcPkW_from_soc_input[i].x_LB = soc_tmp;
            
            line_segment tmp_seg = {soc_0, soc_tmp, 0, dc_pwr_limit_kW};
            dcPkW_from_soc_input.push_back(tmp_seg);
        }
    }
    
    //--------------------------------------
    
    dcPkW_from_soc = dcPkW_from_soc_input;
    
    //=====================================================
    //       Calculate Objective Function Constraints
    //=====================================================
    
    int boundary_index;
    
    std::sort(dcPkW_from_soc_input.begin(), dcPkW_from_soc_input.end());
    
    boundary_index = -1;
    for(int i=dcPkW_from_soc_input.size()-1; i>=0; i--)
    {
        if(0 <= dcPkW_from_soc_input[i].a)
        {
            boundary_index = i;
            break;
        }
    }
    
    //----------------------
    
    std::vector<xy_point> points_A;
    std::vector<xy_point> points_B;
    double x0, x1, y0, y1;
    
    for(int i=0; i<dcPkW_from_soc_input.size(); i++)
    {
        x0 = dcPkW_from_soc_input[i].x_LB;
        y0 = dcPkW_from_soc_input[i].a * x0 + dcPkW_from_soc_input[i].b;
        x1 = dcPkW_from_soc_input[i].x_UB;
        y1 = dcPkW_from_soc_input[i].a * x1 + dcPkW_from_soc_input[i].b;
        w = std::sqrt((x1-x0)*(x1-x0) + (y1-y0)*(y1-y0));
        
        xy_point point_0 = {x0, y0, w};
        xy_point point_1 = {x1, y1, w};
        
        if(i <= boundary_index)
        {
            points_A.push_back(point_0);
            points_A.push_back(point_1);
        }
        else
        {
            points_B.push_back(point_0);
            points_B.push_back(point_1);
        }
    }
    
    lin_reg_slope_yinter  val_A = linear_regression::weighted(points_A);
    lin_reg_slope_yinter  val_B = linear_regression::weighted(points_B);
    
    //----------------------                                               
    
    bat_objfun_constraints constraint_A = {val_A.m, val_A.b};
    bat_objfun_constraints constraint_B = {val_B.m, val_B.b};
    
    std::vector<bat_objfun_constraints> constraints_;
    constraints_.push_back(constraint_A);
    constraints_.push_back(constraint_B);
                          
    constraints = constraints_;
}


void create_dcPkW_from_soc::get_discharging_dcfc_charge_profile(double bat_energy_kWh, double dc_current_limit, double dc_pwr_limit_kW, double bat_capacity_Ah_1C, std::vector<line_segment> &dcPkW_from_soc, std::vector<bat_objfun_constraints> &constraints)
{
    //=====================================================
    //    Interpolate pu curves using dc_current_limit
    //=====================================================
    
    std::vector<line_segment> dcPkW_from_soc_input;
    double soc_0, soc_1, soc_tmp, P_0, P_1, w, P_tmp;
    double m, b, m1, b1;
    
    if(dc_current_limit <= curves[curves.size()-1].get_current_rate_amps(bat_capacity_Ah_1C))
    {
        std::vector<Ppu_vs_soc_point> points;
        points = curves[curves.size()-1].get_points();
        
        for(int i=0; i<points.size()-1; i++)
        {
            m = (points[i].P_pu - points[i+1].P_pu) / (points[i].soc - points[i+1].soc);
            b = points[i].P_pu - m * points[i].soc;
            
            line_segment tmp_seg = {points[i].soc, points[i+1].soc, m, b};            
            dcPkW_from_soc_input.push_back(tmp_seg);
        }
    }
    else
    {
        //--------------------------------------    
        //     Select Curves to Interpolate
        //--------------------------------------
        
        Ppu_vs_soc_curve upper_curve, lower_curve;
        
        for(int i=curves.size()-2; 0<=i; i--)
        {
            if(dc_current_limit < curves[i].get_current_rate_amps(bat_capacity_Ah_1C))
            {
                upper_curve = curves[i+1];
                lower_curve = curves[i];
                break;
            }
        }

        //--------------------------------------    
        //       Interpolate Curves
        //--------------------------------------
        
        // interpolated_val = weight_factor*high_val  +  (1 - weight_factor)*low_val;
        double weight_factor = (dc_current_limit - lower_curve.get_current_rate_amps(bat_capacity_Ah_1C)) / (upper_curve.get_current_rate_amps(bat_capacity_Ah_1C) - lower_curve.get_current_rate_amps(bat_capacity_Ah_1C));
        
        std::vector<Ppu_vs_soc_point> upper_points = upper_curve.get_points();
        std::vector<Ppu_vs_soc_point> lower_points = lower_curve.get_points();
        
        //----------------- 
        
        int index_upper_curve, index_lower_curve;
        double next_soc_lower_curve, next_soc_upper_curve;
        bool extend_segment;
        
        index_upper_curve = upper_points.size() - 1;
        index_lower_curve = lower_points.size() - 1;
        soc_0 = 100;
        P_0 = weight_factor*upper_points[index_upper_curve].P_pu + (1-weight_factor)*lower_points[index_lower_curve].P_pu;
        
        // upper_points and lower_points are points not segments
        while(true)
        {
            next_soc_lower_curve = lower_points[index_lower_curve - 1].soc;
            next_soc_upper_curve = upper_points[index_upper_curve - 1].soc;
            extend_segment = false;
            
            if(std::abs(next_soc_lower_curve - next_soc_upper_curve) < 1)
            {
                if(upper_points[index_upper_curve - 1].point_type == extend) 
                    extend_segment = true;
                
                soc_1 = next_soc_upper_curve;
                P_1 = weight_factor*upper_points[index_upper_curve-1].P_pu + (1-weight_factor)*lower_points[index_lower_curve-1].P_pu;
                index_lower_curve -= 1;
                index_upper_curve -= 1;
            }
            else if(next_soc_upper_curve > next_soc_lower_curve)
            {
                if(upper_points[index_upper_curve - 1].point_type == extend) 
                    extend_segment = true;
                
                soc_1 = next_soc_upper_curve;
                
                w = (soc_1 - lower_points[index_lower_curve].soc) / (lower_points[index_lower_curve-1].soc - lower_points[index_lower_curve].soc);
                P_tmp = w*lower_points[index_lower_curve-1].P_pu + (1-w)*lower_points[index_lower_curve].P_pu;
                    
                P_1 = weight_factor*upper_points[index_upper_curve-1].P_pu + (1-weight_factor)*P_tmp;
                index_upper_curve -= 1;
            }
            else if(next_soc_lower_curve > next_soc_upper_curve)
            {
                soc_1 = next_soc_lower_curve;
                
                w = (soc_1 - upper_points[index_upper_curve].soc) / (upper_points[index_upper_curve-1].soc - upper_points[index_upper_curve].soc);
                P_tmp = w*upper_points[index_upper_curve-1].P_pu + (1-w)*upper_points[index_upper_curve].P_pu;
                    
                P_1 = weight_factor*P_tmp + (1-weight_factor)*lower_points[index_lower_curve-1].P_pu;
                index_lower_curve -= 1;
            }
            
            //-------------------------------
            
            m = (P_1 - P_0) / (soc_1 - soc_0);
            b = P_1 - m*soc_1;
            
            if(extend_segment)
            {
                m1 = (upper_points[index_upper_curve-1].P_pu - upper_points[index_upper_curve].P_pu) / (upper_points[index_upper_curve-1].soc - upper_points[index_upper_curve].soc); 
                b1 = upper_points[index_upper_curve].P_pu - m1*upper_points[index_upper_curve].soc;
                                                 
                soc_1 = (b-b1) / (m1-m);
                P_1 = m*soc_1 + b;
            }
            
            line_segment tmp_seg = {soc_1, soc_0, m, b};
            dcPkW_from_soc_input.push_back(tmp_seg);
            
            soc_0 = soc_1;
            P_0 = P_1;
            
            if(extend_segment)
                break;
        }
        
         //--------------------------------------    
        //       Use Upper Curve Directly
        //--------------------------------------
        
        for(int i=index_upper_curve-1; 0<=i; i--)
        {
            soc_1 = upper_points[i].soc;
            P_1 = upper_points[i].P_pu;
            
            m = (P_1 - P_0) / (soc_1 - soc_0);
            b = P_1 - m*soc_1;
            
            line_segment tmp_seg = {soc_1, soc_0, m, b};
            dcPkW_from_soc_input.push_back(tmp_seg);
            
            soc_0 = soc_1;
            P_0 = P_1;
        }
    }
    
    //=====================================================
    //            Scale Using Battery Energy
    //=====================================================
    
    for(int i=0; i<dcPkW_from_soc_input.size(); i++)
    {
        dcPkW_from_soc_input[i].a *= bat_energy_kWh;
        dcPkW_from_soc_input[i].b *= bat_energy_kWh;
    }
    
    //=====================================================
    //                Apply Power Limits
    //=====================================================
    
    for(int i=0; i<dcPkW_from_soc_input.size(); i++)
    {
        soc_0 = dcPkW_from_soc_input[i].x_LB;
        soc_1 = dcPkW_from_soc_input[i].x_UB;
        m = dcPkW_from_soc_input[i].a;
        b = dcPkW_from_soc_input[i].b;
    
        P_0 = m*soc_0 + b;
        P_1 = m*soc_1 + b;
        
        if(P_0 <= dc_pwr_limit_kW && P_1 <= dc_pwr_limit_kW)
        {
            dcPkW_from_soc_input[i].a = 0;
            dcPkW_from_soc_input[i].b = dc_pwr_limit_kW;        
        }
        else if(dc_pwr_limit_kW < P_0 && P_1 < dc_pwr_limit_kW)
        {
            soc_tmp = (dc_pwr_limit_kW - b)/m;
            dcPkW_from_soc_input[i].x_UB = soc_tmp;
            
            line_segment tmp_seg = {soc_tmp, soc_1, 0, dc_pwr_limit_kW};
            dcPkW_from_soc_input.push_back(tmp_seg);
        }
        else if(P_0 < dc_pwr_limit_kW && dc_pwr_limit_kW < P_1)
        {
            soc_tmp = (dc_pwr_limit_kW - b)/m;
            dcPkW_from_soc_input[i].x_LB = soc_tmp;
            
            line_segment tmp_seg = {soc_0, soc_tmp, 0, dc_pwr_limit_kW};
            dcPkW_from_soc_input.push_back(tmp_seg);
        }
    }
    
    //--------------------------------------
    
    dcPkW_from_soc = dcPkW_from_soc_input;
    
    //=====================================================
    //       Calculate Objective Function Constraints
    //=====================================================
    
    int boundary_index;
    
    std::sort(dcPkW_from_soc_input.begin(), dcPkW_from_soc_input.end());
    
    boundary_index = -1;
    for(int i=0; i<dcPkW_from_soc_input.size(); i++)
    {
        if(0 <= dcPkW_from_soc_input[i].a)
        {
            boundary_index = i;
            break;
        }
    }
    
    //----------------------
    
    std::vector<xy_point> points_A;
    std::vector<xy_point> points_B;
    double x0, x1, y0, y1;
    
    for(int i=0; i<dcPkW_from_soc_input.size(); i++)
    {
        x0 = dcPkW_from_soc_input[i].x_LB;
        y0 = dcPkW_from_soc_input[i].a * x0 + dcPkW_from_soc_input[i].b;
        x1 = dcPkW_from_soc_input[i].x_UB;
        y1 = dcPkW_from_soc_input[i].a * x1 + dcPkW_from_soc_input[i].b;
        w = std::sqrt((x1-x0)*(x1-x0) + (y1-y0)*(y1-y0));
        
        xy_point point_0 = {x0, y0, w};
        xy_point point_1 = {x1, y1, w};
        
        if(i < boundary_index)
        {
            points_A.push_back(point_0);
            points_A.push_back(point_1);
        }
        else
        {
            points_B.push_back(point_0);
            points_B.push_back(point_1);
        }
    }
    
    lin_reg_slope_yinter  val_A = linear_regression::weighted(points_A);
    lin_reg_slope_yinter  val_B = linear_regression::weighted(points_B);
    
    //----------------------                                               
    
    bat_objfun_constraints constraint_A = {val_A.m, val_A.b};
    bat_objfun_constraints constraint_B = {val_B.m, val_B.b};
    
    std::vector<bat_objfun_constraints> constraints_;
    constraints_.push_back(constraint_A);
    constraints_.push_back(constraint_B);
                          
    constraints = constraints_;
}


//=============================================================================
//                       pev_charge_profile_factory
//=============================================================================
                          
//=============================================
//                Init State Vals
//=============================================

pev_charge_profile_factory::pev_charge_profile_factory()
{   
    std::vector<Ppu_vs_soc_point> points;
    std::vector<Ppu_vs_soc_curve> curves;    
    double C_rate;
    
    bool is_charging_not_discharging = true;
	
    //========================
    //    LMO  Charging
    //========================
       
    curves.clear();
    
                    // {soc, P, (interpolate, extend, use_directly)}
    points.clear();
    points.push_back({  0.0, 0.898, interpolate});
    points.push_back({  4.4, 1.056, interpolate});
    points.push_back({ 11.3, 1.154, interpolate});
    points.push_back({ 32.4, 1.215, interpolate});
    points.push_back({ 76.1, 1.274, extend});
    points.push_back({100.0, 0.064, use_directly});
    
    C_rate = 1;
    Ppu_vs_soc_curve curve_01(C_rate, points);
    curves.push_back(curve_01);

    //------------------------------------
                    
                   // {soc, P, (interpolate, extend, use_directly)}
    points.clear();
    points.push_back({  0.0, 1.742, interpolate});
    points.push_back({  4.0, 2.044, interpolate});
    points.push_back({ 12.0, 2.249, interpolate});
    points.push_back({ 55.0, 2.418, extend});
    points.push_back({ 75.0, 1.190, use_directly});
    points.push_back({100.0, 0.064, use_directly});
    
    C_rate = 2;
    Ppu_vs_soc_curve curve_02(C_rate, points);
    curves.push_back(curve_02);

    //------------------------------------
    
                   // {soc, P, (interpolate, extend, use_directly)}
    points.clear();
    points.push_back({  0.0, 2.667, interpolate});
    points.push_back({  6.0, 3.246, interpolate});
    points.push_back({ 11.9, 3.436, interpolate});
    points.push_back({ 37.6, 3.628, extend});
    points.push_back({ 70.0, 1.440, use_directly});
    points.push_back({100.0, 0.064, use_directly});
    
    C_rate = 3;
    Ppu_vs_soc_curve curve_03(C_rate, points);
    curves.push_back(curve_03);
    
    //------------------------------------
    
                  // {soc, P, (interpolate, extend, use_directly)}
    points.clear();
    points.push_back({  0.0, 0, interpolate});
    points.push_back({100.0, 0, interpolate});
    
    C_rate = 0;
    Ppu_vs_soc_curve curve_04(C_rate, points);
    curves.push_back(curve_04);
    
    //------------------------------------
                    
    create_dcPkW_from_soc LMO_charge_(is_charging_not_discharging, curves);
    this->LMO_charge = LMO_charge_;
        
    //========================
    //    NMC  Charging
    //========================
    curves.clear();
    
                    // {soc, P, (interpolate, extend, use_directly)}
    points.clear();
    points.push_back({  0.0, 0.917, interpolate});
    points.push_back({  4.0, 1.048, interpolate});
    points.push_back({ 10.0, 1.095, interpolate});
    points.push_back({ 88.0, 1.250, extend});
    // points.push_back({ 93.0, 0.595, use_directly});  // This point violates one of the key rules.
    points.push_back({100.0, 0.060, use_directly});
    
    C_rate = 1;
    Ppu_vs_soc_curve curve_11(C_rate, points);
    curves.push_back(curve_11);

    //------------------------------------
                    
                   // {soc, P, (interpolate, extend, use_directly)}
    points.clear();
    points.push_back({  0.0, 1.750, interpolate});
    points.push_back({  3.0, 2.000, interpolate});
    points.push_back({ 10.0, 2.143, interpolate});
    points.push_back({ 78.5, 2.417, extend});
    points.push_back({ 93.0, 0.595, use_directly});
    points.push_back({100.0, 0.060, use_directly});
    
    C_rate = 2;
    Ppu_vs_soc_curve curve_12(C_rate, points);
    curves.push_back(curve_12);

    //------------------------------------
    
                   // {soc, P, (interpolate, extend, use_directly)}
    points.clear();
    points.push_back({  0.0, 2.798, interpolate});
    points.push_back({  3.0, 3.167, interpolate});
    points.push_back({ 10.0, 3.393, interpolate});
    points.push_back({ 67.0, 3.750, extend});
    points.push_back({ 93.0, 0.595, use_directly});
    points.push_back({100.0, 0.060, use_directly});
    
    C_rate = 3;
    Ppu_vs_soc_curve curve_13(C_rate, points);
    curves.push_back(curve_13);
    
    //------------------------------------
    
                  // {soc, P, (interpolate, extend, use_directly)}
    points.clear();
    points.push_back({  0.0, 0, interpolate});
    points.push_back({100.0, 0, interpolate});
    
    C_rate = 0;
    Ppu_vs_soc_curve curve_14(C_rate, points);
    curves.push_back(curve_14);
    
    //------------------------------------
                    
    create_dcPkW_from_soc NMC_charge_(is_charging_not_discharging, curves);
    this->NMC_charge = NMC_charge_;
    
    //========================
    //    LTO  Charging
    //========================
    curves.clear();
    
                    // {soc, P, (interpolate, extend, use_directly)}
    points.clear();
    points.push_back({  0.0, 0.798, interpolate});
    points.push_back({  2.0, 0.882, interpolate});
    points.push_back({ 50.0, 0.966, interpolate});
    points.push_back({ 64.0, 1.008, interpolate});
    points.push_back({ 80.0, 1.040, interpolate});
    points.push_back({ 90.0, 1.071, interpolate});
    points.push_back({ 96.0, 1.134, extend});
    points.push_back({100.0, 0.057, use_directly});
    
    C_rate = 1;
    Ppu_vs_soc_curve curve_21(C_rate, points);
    curves.push_back(curve_21);
	
    //------------------------------------
                    
                   // {soc, P, (interpolate, extend, use_directly)}
    points.clear();
    points.push_back({  0.0, 1.765, interpolate});
    points.push_back({  2.0, 1.828, interpolate});
    points.push_back({ 50.0, 1.975, interpolate});
    points.push_back({ 60.0, 2.038, interpolate});
    points.push_back({ 80.0, 2.122, interpolate});
    points.push_back({ 91.0, 2.227, extend});
    points.push_back({100.0, 0.057, use_directly});
    
    C_rate = 2;
    Ppu_vs_soc_curve curve_22(C_rate, points);
    curves.push_back(curve_22);

    //------------------------------------
    
                   // {soc, P, (interpolate, extend, use_directly)}
    points.clear();
    points.push_back({  0.0, 2.647, interpolate});
    points.push_back({  2.0, 2.794, interpolate});
    points.push_back({ 50.0, 2.983, interpolate});
    points.push_back({ 60.0, 3.109, interpolate});
    points.push_back({ 80.0, 3.256, interpolate});
    points.push_back({ 88.0, 3.361, extend});
    points.push_back({100.0, 0.085, use_directly});
    
    C_rate = 3;
    Ppu_vs_soc_curve curve_23(C_rate, points);
    curves.push_back(curve_23);
    
    //------------------------------------
    
                   // {soc, P, (interpolate, extend, use_directly)}
    points.clear();
    points.push_back({  0.0, 3.655, interpolate});
    points.push_back({  3.0, 3.782, interpolate});
    points.push_back({ 50.0, 4.055, interpolate});
    points.push_back({ 60.0, 4.202, interpolate});
    points.push_back({ 80.0, 4.391, interpolate});
    points.push_back({ 86.0, 4.517, extend});
    points.push_back({100.0, 0.113, use_directly});
    
    C_rate = 4;
    Ppu_vs_soc_curve curve_24(C_rate, points);
    curves.push_back(curve_24);
    
    //------------------------------------
    
                   // {soc, P, (interpolate, extend, use_directly)}
    points.clear();
    points.push_back({  0.0, 4.622, interpolate});
    points.push_back({  4.0, 4.832, interpolate});
    points.push_back({ 50.0, 5.168, interpolate});
    points.push_back({ 60.0, 5.357, interpolate});
    points.push_back({ 84.0, 5.630, extend});
    points.push_back({100.0, 0.063, use_directly});
    
    C_rate = 5;
    Ppu_vs_soc_curve curve_25(C_rate, points);
    curves.push_back(curve_25);
    
    //------------------------------------
    
                  // {soc, P, (interpolate, extend, use_directly)}
    points.clear();
    points.push_back({  0.0, 0, interpolate});
    points.push_back({100.0, 0, interpolate});
    
    C_rate = 0;
    Ppu_vs_soc_curve curve_26(C_rate, points);
    curves.push_back(curve_26);
    
    //------------------------------------
                    
    create_dcPkW_from_soc LTO_charge_(is_charging_not_discharging, curves);
    this->LTO_charge = LTO_charge_;
}
    

std::vector<line_segment> pev_charge_profile_factory::get_dcfc_charge_profile(pev_battery_chemistry bat_chemistry, double battery_size_kWh, double bat_capacity_Ah_1C, double pev_max_c_rate, double pev_P2_pwr_limit_kW, double dcfc_dc_current_limit)
{
	std::vector<line_segment> pev_charge_profile;
	
	double pev_dc_current_limit = pev_max_c_rate * bat_capacity_Ah_1C;
    double dc_current_limit = pev_dc_current_limit < dcfc_dc_current_limit ? pev_dc_current_limit : dcfc_dc_current_limit;

	if(bat_chemistry == LMO)
		this->LMO_charge.get_dcfc_charge_profile(battery_size_kWh, dc_current_limit, pev_P2_pwr_limit_kW, bat_capacity_Ah_1C, pev_charge_profile, this->constraints);

	else if(bat_chemistry == LTO)
		this->LTO_charge.get_dcfc_charge_profile(battery_size_kWh, dc_current_limit, pev_P2_pwr_limit_kW, bat_capacity_Ah_1C, pev_charge_profile, this->constraints);

	else if(bat_chemistry == NMC)
		this->NMC_charge.get_dcfc_charge_profile(battery_size_kWh, dc_current_limit, pev_P2_pwr_limit_kW, bat_capacity_Ah_1C, pev_charge_profile, this->constraints);
    
    return pev_charge_profile;
}


std::vector<bat_objfun_constraints> pev_charge_profile_factory::get_constraints()
{
	return this->constraints;
}


std::vector<line_segment> pev_charge_profile_factory::get_L1_or_L2_charge_profile(pev_battery_chemistry bat_chemistry, double bat_energy_kWh, double pev_charge_rate_kW)
{
	std::vector<line_segment> pev_charge_profile;
	
	if(bat_chemistry == LMO)
		this->LMO_charge.get_L1_or_L2_charge_profile(bat_energy_kWh, pev_charge_rate_kW, pev_charge_profile);

	else if(bat_chemistry == LTO)
		this->LTO_charge.get_L1_or_L2_charge_profile(bat_energy_kWh, pev_charge_rate_kW, pev_charge_profile);

	else if(bat_chemistry == NMC)
		this->NMC_charge.get_L1_or_L2_charge_profile(bat_energy_kWh, pev_charge_rate_kW, pev_charge_profile);
		
	return pev_charge_profile;
}


////////////////////////////////////////////////////////////////////////////////////////////////////

/*
int main(int args, char* argv[])
{
	std::vector<line_segment> pev_charge_profile;
	pev_battery_chemistry bat_chemistry = LTO;
	double bat_energy_kWh = 65;
	double charge_rate_kW = 7.6;
	
	pev_charge_profile_factory factory;
	factory.get_L1_or_L2_charge_profile(bat_chemistry, bat_energy_kWh, charge_rate_kW, pev_charge_profile);
	
	return 0;
}
*/
