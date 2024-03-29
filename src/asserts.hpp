#ifndef ASSERTS_HPP_INCLUDED
#define ASSERTS_HPP_INCLUDED

#include <iostream>
#include <stdlib.h>
#include <string>

//various asserts of standard "equality" tests, such as "equals", "not equals", "greater than", etc.  Example usage:
//ASSERT_NE(x, y);
#define ASSERT_EQ(a,b) if((a) != (b)) { std::cerr << __FILE__ << ":" << __LINE__ << " ASSERT EQ FAILED: " << #a << " != " << #b << ": " << (a) << " != " << (b) << "\n"; abort(); }

#define ASSERT_NE(a,b) if((a) == (b)) { std::cerr << __FILE__ << ":" << __LINE__ << " ASSERT NE FAILED: " << #a << " == " << #b << ": " << (a) << " == " << (b) << "\n"; abort(); }

#define ASSERT_GE(a,b) if((a) < (b)) { std::cerr << __FILE__ << ":" << __LINE__ << " ASSERT GE FAILED: " << #a << " < " << #b << ": " << (a) << " < " << (b) << "\n"; abort(); }

#define ASSERT_LE(a,b) if((a) > (b)) { std::cerr << __FILE__ << ":" << __LINE__ << " ASSERT LE FAILED: " << #a << " > " << #b << ": " << (a) << " > " << (b) << "\n"; abort(); }

#define ASSERT_GT(a,b) if((a) <= (b)) { std::cerr << __FILE__ << ":" << __LINE__ << " ASSERT GT FAILED: " << #a << " <= " << #b << ": " << (a) << " <= " << (b) << "\n"; abort(); }

#define ASSERT_LT(a,b) if((a) >= (b)) { std::cerr << __FILE__ << ":" << __LINE__ << " ASSERT LT FAILED: " << #a << " >= " << #b << ": " << (a) << " >= " << (b) << "\n"; abort(); }

#define ASSERT_INDEX_INTO_VECTOR(a,b) if((a) < 0 || (a) >= (b).size()) { std::cerr << __FILE__ << ":" << __LINE__ << " ASSERT INDEX INTO VECTOR FAILED: " << #a << " indexes " << #b << ": " << (a) << " indexes " << (b).size(); abort(); }

//for custom logging.  Example usage:
//ASSERT_LOG(x != y, "x not equal to y. Value of x: " << x << ", y: " << y);
#define ASSERT_LOG(a,b) if( !(a) ) { std::cerr << __FILE__ << ":" << __LINE__ << " ASSERTION FAILED: " << b << "\n"; abort(); }


struct assert_fail_exception {};

//various asserts of standard "equality" tests, such as "equals", "not equals", "greater than", etc.  Example usage:
//EXPECT_NE(x, y);
#define EXPECT_EQ(a,b) if((a) != (b)) { std::cerr << __FILE__ << ":" << __LINE__ << " EXPECT EQ FAILED: " << #a << " != " << #b << ": " << (a) << " != " << (b) << "\n"; throw assert_fail_exception(); }

#define EXPECT_NE(a,b) if((a) == (b)) { std::cerr << __FILE__ << ":" << __LINE__ << " EXPECT NE FAILED: " << #a << " == " << #b << ": " << (a) << " == " << (b) << "\n"; throw assert_fail_exception(); }

#define EXPECT_GE(a,b) if((a) < (b)) { std::cerr << __FILE__ << ":" << __LINE__ << " EXPECT GE FAILED: " << #a << " < " << #b << ": " << (a) << " < " << (b) << "\n"; throw assert_fail_exception(); }

#define EXPECT_LE(a,b) if((a) > (b)) { std::cerr << __FILE__ << ":" << __LINE__ << " EXPECT LE FAILED: " << #a << " > " << #b << ": " << (a) << " > " << (b) << "\n"; throw assert_fail_exception(); }

#define EXPECT_GT(a,b) if((a) <= (b)) { std::cerr << __FILE__ << ":" << __LINE__ << " EXPECT GT FAILED: " << #a << " <= " << #b << ": " << (a) << " <= " << (b) << "\n"; throw assert_fail_exception(); }

#define EXPECT_LT(a,b) if((a) >= (b)) { std::cerr << __FILE__ << ":" << __LINE__ << " EXPECT LT FAILED: " << #a << " >= " << #b << ": " << (a) << " >= " << (b) << "\n"; throw assert_fail_exception(); }

#define EXPECT_INDEX_INTO_VECTOR(a,b) if((a) < 0 || (a) >= (b).size()) { std::cerr << __FILE__ << ":" << __LINE__ << " EXPECT INDEX INTO VECTOR FAILED: " << #a << " indexes " << #b << ": " << (a) << " indexes " << (b).size(); throw assert_fail_exception(); }

//for custom logging.  Example usage:
//EXPECT_LOG(x != y, "x not equal to y. Value of x: " << x << ", y: " << y);
#define EXPECT_LOG(a,b) if( !(a) ) { std::cerr << __FILE__ << ":" << __LINE__ << " EXPECT LOG FAILED: " << b << "\n"; throw assert_fail_exception(); }

#endif
