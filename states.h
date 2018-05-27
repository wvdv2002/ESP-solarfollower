
const char* sunState_str[] = {"IDLE","MOVEND","MOVTIMEOUT","WAITSUNDOWN","MOVBEGIN","WAITSUNRISE"};
typedef enum sunstate {
  SUN_IDLE,
  SUN_MOVING_TOEND,
  SUN_MOVE_TIMEOUT,
  SUN_WAITINGFORSUNDOWN,
  SUN_MOVING_TOBEGIN,
  SUN_WAITINGFORSUNRISE,  
}sunState_t;

const char* platformState_str[] = {"INBETWEEN","ATEND","ATBEGIN","IDLE"};
typedef enum platformstate {
  PLATFORM_INBETWEEN,
  PLATFORM_ATEND,
  PLATFORM_ATBEGIN,
  PLATFORM_IDLE,
}platformState_t;

const char* platformMovingState_str[] = {"IDLE","TOBEGIN","TOEND"};

typedef enum platformmovingstate {
  PLATFORM_STOPPING,
  PLATFORM_MOVINGTOBEGIN,
  PLATFORM_MOVINGTOEND,
}platformMovingState_t;

const char* sunSensorState_str[] = {"MEASBEG","MEASEND","IDLE"};
typedef enum sunsensorstate{
  SUNSENSOR_MEASURING_TOBEGIN,
  SUNSENSOR_MEASURING_TOEND,
  SUNSENSOR_IDLE,
}sunSensorState_t;


