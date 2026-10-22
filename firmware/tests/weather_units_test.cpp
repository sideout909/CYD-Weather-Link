#include "weather_units.h"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
 using namespace weather_units;
 auto near=[](double a,double b){return std::abs(a-b)<0.00001;};
 assert(valid("metric")&&valid("imperial")&&!valid(nullptr)&&!valid("SI"));
 assert(near(temperature(32,true),0)&&near(temperature(-40,true),-40)&&near(temperature(212,true),100));
 assert(near(wind(0,true),0)&&near(wind(10,true),16.09344));
 assert(near(rain(1,true),25.4)&&near(pressure(101325,true),1013.25));
 assert(near(temperature(32,false),32)&&near(wind(10,false),10)&&near(rain(1,false),1));
 assert(near(pressure(3386.389,false),1));
 assert(windDescription("SW 5 to 10 mph",true)=="SW 8 to 16 km/h");
 assert(windDescription("NW 0 mph",true)=="NW 0 km/h");
 assert(windDescription("Calm",true)=="Calm");
 assert(windDescription("5 to 10 mph",false)=="5 to 10 mph");
 assert(windDescription("wind unavailable",true)=="wind unavailable");
 std::cout<<"Units checks passed: freezing, boiling, negative/zero, pressure, rain, wind and NWS descriptors.\n";
}
