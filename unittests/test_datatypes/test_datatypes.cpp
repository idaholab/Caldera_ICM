#include "datatypes_global.h"

class test_datatypes
{
public:

    static int test_hello_world()
    {
        std::cout << "Hello, world [test_datatypes]!" << std::endl;
        return 0;
    }
    
    static int test_time_series_v2_001()
    {
        // In the event the test is a failure, the exit-code will be
        // changed to a non-zero value.
        int exit_code = 0;
        
        // --- helper function ---
        const auto assert_bool_true = [&exit_code] ( const bool b, const std::string error_msg_if_false )
        {
            if( !b )
            {
                exit_code++;
                std::cout << error_msg_if_false << std::endl;
            }
        };
        
        std::cout << std::endl;
        std::cout << std::endl;
        std::cout << "test_time_series_v2_001" << std::endl;
        std::cout << std::endl;
        
        const std::vector<int> idata =    { 99,  20,  17,  64,  28  };
        const std::vector<float> fdata =  { 1.0, 3.0, 4.4, 3.2, 7.8 };
        const std::vector<double> ddata = { 2.0, 4.0, 5.4, 4.2, 8.9 };
        
        time_series_v2<int> ts0( 0.0, 0.1, idata );
        time_series_v2<float> ts1( 0.0, 0.1,fdata );
        time_series_v2<double> ts2( 0.0, 0.1, ddata );
        
        std::cout << "ts0.data.at(3): " << ts0.data.at(3) << std::endl;
        std::cout << "ts1.data.at(3): " << ts1.data.at(3) << std::endl;
        std::cout << "ts2.data.at(3): " << ts2.data.at(3) << std::endl;
        std::cout << std::endl;
        
        const float ieps = 0;
        const float feps = 1e-9;
        const float deps = 1e-14;
        
        assert_bool_true( ts0.data.at(3) == (int)64, "Error: wrong value (int)." );
        assert_bool_true( fabs( ts1.data.at(3) - (float)3.2 ) < feps, "Error: wrong value (float)." );
        assert_bool_true( fabs( ts2.data.at(3) - (double)4.2 ) < deps, "Error: wrong value (double)." );
        
        assert_bool_true( ts0.get_val_from_time_with_error_check( 0.1000001 ) == (int)20,  "Error: wrong value when testing 'get_val_from_time_with_error_check' (int)." );
        assert_bool_true( fabs( ts1.get_val_from_time_with_error_check( 0.1000001 ) - (float)3.0 ) < feps,   "Error: wrong value when testing 'get_val_from_time_with_error_check' (float)." );
        assert_bool_true( fabs( ts2.get_val_from_time_with_error_check( 0.1000001 ) - (double)4.0 ) < deps,  "Error: wrong value when testing 'get_val_from_time_with_error_check' (double)." );
        
        assert_bool_true( ts0.get_val_from_time_with_cycling( 0.7000001 ) == (int)17,  "Error: wrong value when testing 'get_val_from_time_with_cycling' (int)." );
        assert_bool_true( fabs( ts1.get_val_from_time_with_cycling( 0.7000001 ) - (float)4.4 ) < feps,   "Error: wrong value when testing 'get_val_from_time_with_cycling' (float)." );
        assert_bool_true( fabs( ts2.get_val_from_time_with_cycling( 0.7000001 ) - (double)5.4 ) < deps,  "Error: wrong value when testing 'get_val_from_time_with_cycling' (double)." );
        
        assert_bool_true( ts0.get_val_from_time_with_default( 0.1000001, 45 ) == (int)20,  "Error: wrong value when testing 'get_val_from_time_with_default' (int) [1]." );
        assert_bool_true( fabs( ts1.get_val_from_time_with_default( 0.1000001, 45.0 ) - (float)3.0 ) < feps,   "Error: wrong value when testing 'get_val_from_time_with_default' (float) [1]." );
        assert_bool_true( fabs( ts2.get_val_from_time_with_default( 0.1000001, 45.0 ) - (double)4.0 ) < deps,  "Error: wrong value when testing 'get_val_from_time_with_default' (double) [1]." );
        assert_bool_true( ts0.get_val_from_time_with_default( 0.7000001, 45 ) == (int)45,  "Error: wrong value when testing 'get_val_from_time_with_default' (int) [2]." );
        assert_bool_true( fabs( ts1.get_val_from_time_with_default( 0.7000001, 45.0 ) - (float)45.0 ) < feps,   "Error: wrong value when testing 'get_val_from_time_with_default' (float) [2]." );
        assert_bool_true( fabs( ts2.get_val_from_time_with_default( 0.7000001, 45.0 ) - (double)45.0 ) < deps,  "Error: wrong value when testing 'get_val_from_time_with_default' (double) [2]." );

        assert_bool_true( ts0.get_val_from_index( 2 ) == (int)17,  "Error: wrong value when testing 'get_val_from_index' (int)." );
        assert_bool_true( fabs( ts1.get_val_from_index( 2 ) - (float)4.4 ) < feps,   "Error: wrong value when testing 'get_val_from_index' (float)." );
        assert_bool_true( fabs( ts2.get_val_from_index( 2 ) - (double)5.4 ) < deps,  "Error: wrong value when testing 'get_val_from_index' (double)." );
        
        assert_bool_true( ts0.get_time_sec_from_index( 2 ) == (double)0.2, "Error: wrong value when testing 'get_time_sec_from_index' (int)." );
        assert_bool_true( ts1.get_time_sec_from_index( 2 ) == (double)0.2, "Error: wrong value when testing 'get_time_sec_from_index' (float)." );
        assert_bool_true( ts2.get_time_sec_from_index( 2 ) == (double)0.2, "Error: wrong value when testing 'get_time_sec_from_index' (double)." );

        assert_bool_true( ts0.get_index_from_time( 0.1 ) == 1, "Error: wrong value when testing 'get_index_from_time' (int)." );
        assert_bool_true( ts0.get_index_from_time( 0.199998 ) == 1, "Error: wrong value when testing 'get_index_from_time' (int)." );
        assert_bool_true( ts0.get_index_from_time( 0.3003 ) == 3, "Error: wrong value when testing 'get_index_from_time' (int)." );
        
        assert_bool_true( ts1.get_index_from_time( 0.2 ) == 2, "Error: wrong value when testing 'get_index_from_time' (float)." );
        assert_bool_true( ts1.get_index_from_time( 0.399998 ) == 3, "Error: wrong value when testing 'get_index_from_time' (float)." );
        assert_bool_true( ts1.get_index_from_time( 0.4003 ) == 4, "Error: wrong value when testing 'get_index_from_time' (float)." );
        
        assert_bool_true( ts2.get_index_from_time( 0.00123 ) == 0, "Error: wrong value when testing 'get_index_from_time' (double)." );
        assert_bool_true( ts2.get_index_from_time( 0.11001 ) == 1, "Error: wrong value when testing 'get_index_from_time' (double)." );
        assert_bool_true( ts2.get_index_from_time( 0.499998 ) == 4, "Error: wrong value when testing 'get_index_from_time' (double)." );
                
        return exit_code;
    }
};

int main(int argc, char* argv[])
{
    int sum = 0;
    sum += test_datatypes::test_hello_world();
    sum += test_datatypes::test_time_series_v2_001();
    return sum;
}

