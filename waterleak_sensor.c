#include "homeheartbeat.h"
#include "waterleak_sensor.h"


static  char    *deviceNames[] = { 
  "Water Sensor",   "Basement Floor",   "Bathroom Floor",   "Shower",   "Sump Pump",
    "Toilet",       "Washing Machine",  "Water Heater",     "Attic",    "Bedroom",
    "Crawl Space",  "Dining Room",      "Garage",           "Laundry Room", "Living Room",
    "Loft",         "Mud Room",         "Storage Area",     "Bathroom Sink",    "Kitchen Sink",
    "Under Sink",   "Utility Sink",     "Cat Bowl",         "Dog Bowl",         "Fish Tank",
    "Fountain",     "Garden",           "Pool",             "Water Bowl",       "???"  }; 

// ----------------------------------------------------------------------------
void    WaterLeak_parseOneStateRecord (HomeHeartBeatSystem_t *aSystem, char *token[] )
{
    
}