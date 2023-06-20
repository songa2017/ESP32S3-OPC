#include "configure.h"
#ifdef USE_OPC

#include <pthread.h>
#include "open62541.h"
#include "opc.h"
#include "nvs_flash.h"
#include "nvs.h"


bool g_bRunning  = false;
UA_NodeId   g_lastNodeId;
static UA_Boolean running = true;

static UA_Boolean isServerCreated = false;

int NodeidHeartbeat = 0;
const static char* OPC_TAG = "OPC";
static UA_UsernamePasswordLogin logins[3] = {
    {UA_STRING_STATIC("karl690"), UA_STRING_STATIC("karl690")},
    {UA_STRING_STATIC("hyrel"), UA_STRING_STATIC("hyrel")}
};

static UA_StatusCode
UA_ServerConfig_setUriName(UA_ServerConfig *uaServerConfig, const char *uri, const char *name)
{
    // delete pre-initialized values
    UA_String_clear(&uaServerConfig->applicationDescription.applicationUri);
    UA_LocalizedText_clear(&uaServerConfig->applicationDescription.applicationName);

    uaServerConfig->applicationDescription.applicationUri = UA_String_fromChars(uri);
    uaServerConfig->applicationDescription.applicationName.locale = UA_STRING_NULL;
    uaServerConfig->applicationDescription.applicationName.text = UA_String_fromChars(name);

    for (size_t i = 0; i < uaServerConfig->endpointsSize; i++)
    {
        UA_String_clear(&uaServerConfig->endpoints[i].server.applicationUri);
        UA_LocalizedText_clear(
            &uaServerConfig->endpoints[i].server.applicationName);

        UA_String_copy(&uaServerConfig->applicationDescription.applicationUri,
                       &uaServerConfig->endpoints[i].server.applicationUri);

        UA_LocalizedText_copy(&uaServerConfig->applicationDescription.applicationName,
                              &uaServerConfig->endpoints[i].server.applicationName);
    }

    return UA_STATUSCODE_GOOD;
}

void
addRelay0ControlNode(UA_Server *server) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "Relay0");
    attr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
	//attr.value = UA_STRING("ABCD");

    UA_NodeId currentNodeId = UA_NODEID_STRING(1, "Control Relay number 0.");
    UA_QualifiedName currentName = UA_QUALIFIEDNAME(1, "Control Relay number 0.");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId variableTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE);

	UA_DataSource relay0 = { };
    //Configure GPIOs just before adding relay 0 - not a good practice.
    //configureGPIO();
    //relay0.read = readRelay0State;
    //relay0.write = setRelay0State;
    UA_Server_addDataSourceVariableNode(server, currentNodeId, parentNodeId,
                                        parentReferenceNodeId, currentName,
                                        variableTypeNodeId, attr,
                                        relay0, NULL, NULL);
}


int addNodeObject(UA_Server *server, int parentid, const char* objectName, const char* description) {
	UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
	UA_NodeId   NodeId;
	oAttr.description = UA_LOCALIZEDTEXT((char*)"en-US", (char*)description);
	oAttr.displayName = UA_LOCALIZEDTEXT((char*)"en-US", (char*)objectName);
	UA_Server_addObjectNode(server,
		UA_NODEID_NULL,
		UA_NODEID_NUMERIC(0, parentid), //UA_NS0ID_OBJECTSFOLDER),
		UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
		UA_QUALIFIEDNAME(1, (char*)objectName),
		UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
		oAttr,
		NULL,
		&NodeId);
	//lastNodeId = outNodeId;
	return NodeId.identifier.numeric;
}



int addVariable(UA_Server *server, int parentid, const char* variableName, int dataType, bool bWritable) {
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	//UA_Variant_init(&attr.value);
	attr.description = UA_LOCALIZEDTEXT((char*)"en-US", (char*)variableName);
	attr.displayName = UA_LOCALIZEDTEXT((char*)"en-US", (char*)variableName);
	attr.dataType = UA_TYPES[dataType].typeId;
//
//	switch (dataType)
//	{
//	case UA_TYPES_STRING:
//		{
//			UA_String val = UA_STRING((char*)value);
//			UA_Variant_setScalar(&attr.value, &val, &UA_TYPES[UA_TYPES_STRING]);
//		}
//		break;
//	case UA_TYPES_FLOAT:
//		{
//			UA_Float val = atof(value);
//			UA_Variant_setScalar(&attr.value, &val, &UA_TYPES[UA_TYPES_FLOAT]);
//		}
//		break;
//	case UA_TYPES_DOUBLE:
//		{
//			UA_Double val = atof(value);
//			UA_Variant_setScalar(&attr.value, &val, &UA_TYPES[UA_TYPES_DOUBLE]);
//		}
//		break;
//	case UA_TYPES_INT16:
//	case UA_TYPES_INT32:
//		{
//			UA_Double val = atoi(value);
//			UA_Variant_setScalar(&attr.value, &val, &UA_TYPES[UA_TYPES_INT32]);
//		}
//		break;
//	}
	if (bWritable)
		attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
	else
		attr.accessLevel = UA_ACCESSLEVELMASK_READ;
	UA_NodeId childId;
	UA_Server_addVariableNode(server,
		UA_NODEID_NULL, //UA_NODEID_STRING(1, (char*)variableName), //
		UA_NODEID_NUMERIC(0, parentid),
		UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), // UA_NS0ID_ORGANIZES
		UA_QUALIFIEDNAME(1, (char*)variableName),
		UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
		attr,
		NULL,
		&childId);

	//UA_Server_setVariableNode_valueCallback(server, childId, callback);

	return childId.identifier.numeric;

}

bool WriteIntVariable(UA_Server *server, int nodeId, int value)
{
	UA_NodeId NodeId = UA_NODEID_NUMERIC(0, nodeId);
	UA_VariableAttributes Attr;
	UA_Variant_init(&Attr.value);
	UA_Variant_setScalar(&Attr.value, &value, &UA_TYPES[UA_TYPES_INT32]); //UA_TYPES_INT32
	UA_StatusCode status = UA_Server_writeValue(server, NodeId, Attr.value);
	if (status != UA_STATUSCODE_GOOD) {
		return false;
	}
	return true;
}

bool WriteFloatVariable(UA_Server *server, int nodeId, float value)
{
	UA_NodeId NodeId = UA_NODEID_NUMERIC(0, nodeId);
	UA_VariableAttributes Attr;
	UA_Variant_init(&Attr.value);
	UA_Variant_setScalar(&Attr.value, &value, &UA_TYPES[UA_TYPES_FLOAT]); //UA_TYPES_INT32
	UA_StatusCode status = UA_Server_writeValue(server, NodeId, Attr.value);
	if (status != UA_STATUSCODE_GOOD) {
		return false;
	}
	return true;
}
bool WriteDoubleVariable(UA_Server *server, int nodeId, double value)
{
	UA_NodeId NodeId = UA_NODEID_NUMERIC(0, nodeId);
	UA_VariableAttributes Attr;
	UA_Variant_init(&Attr.value);
	UA_Variant_setScalar(&Attr.value, &value, &UA_TYPES[UA_TYPES_DOUBLE]); //UA_TYPES_INT32
	UA_StatusCode status = UA_Server_writeValue(server, NodeId, Attr.value);
	if (status != UA_STATUSCODE_GOOD) {
		return false;
	}
	return true;
}

bool WriteBooleanVariable(UA_Server *server, int nodeId, bool value)
{
	UA_NodeId NodeId = UA_NODEID_NUMERIC(0, nodeId);
	UA_VariableAttributes Attr;
	UA_Variant_init(&Attr.value);
	UA_Variant_setScalar(&Attr.value, &value, &UA_TYPES[UA_TYPES_BOOLEAN]); //UA_TYPES_INT32
	UA_StatusCode status = UA_Server_writeValue(server, NodeId, Attr.value);
	if (status != UA_STATUSCODE_GOOD) {
		return false;
	}
	return true;
}

bool WriteStringVariable(UA_Server *server, int nodeId, char* value)
{
	UA_NodeId NodeId = UA_NODEID_NUMERIC(0, nodeId);
	UA_VariableAttributes Attr;
	UA_Variant_init(&Attr.value);	
	UA_String s = UA_STRING_ALLOC((char*)value);
	UA_Variant_setScalar(&Attr.value, &s, &UA_TYPES[UA_TYPES_STRING]); //UA_TYPES_INT32
	UA_StatusCode status = UA_Server_writeValue(server, NodeId, Attr.value);
	if (status != UA_STATUSCODE_GOOD) {
		return false;
	}
	return true;
}


bool writeVariable(UA_Server *server, int nodeId, const char* val, int type) {	
	UA_NodeId myIntegerNodeId = UA_NODEID_NUMERIC(0, nodeId);
	int valType = UA_TYPES_STRING;

	void* value = NULL;
	float f;
	double d;
	bool b;
	int i;
	switch (type)
	{
	case UA_TYPES_STRING: value = (void*)val; break;
	case UA_TYPES_FLOAT: 
		f = atof(val);
		value = &f;
		break;
	case UA_TYPES_DOUBLE:
		f = atof(val);
		value = &f;
		break;
	case UA_TYPES_BOOLEAN:
		b = atoi(val);
		value = &b;
		break;
	case UA_TYPES_INT16:
		i = atoi(val);
		value = &i;
		break;
	}
	
	/* Write a different integer value */
	UA_VariableAttributes Attr;
	UA_Variant_init(&Attr.value);
	if (valType == UA_TYPES_STRING) {
		UA_String s = UA_STRING_ALLOC((char*)value);
		UA_Variant_setScalar(&Attr.value, &s, &UA_TYPES[valType]); //UA_TYPES_INT32
	}
	else
		UA_Variant_setScalar(&Attr.value, value, &UA_TYPES[valType]); //UA_TYPES_INT32
	
	UA_StatusCode status = UA_Server_writeValue(server, myIntegerNodeId, Attr.value);
	if (status != UA_STATUSCODE_GOOD) {
		//std::cout << "Write Value Error: " << status << "\n";
		return false;
	}
	return true;
}

void thread_opc_task(void* arg) {
    UA_Int32 sendBufferSize = 16384;
    UA_Int32 recvBufferSize = 16384;    
    
	ESP_LOGI(OPC_TAG, "Fire up OPC UA Server.");
    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setMinimalCustomBuffer(config, 4840, 0, sendBufferSize, recvBufferSize);

    const char *appUri = "open62541.esp321.server";
    UA_String hostName = UA_STRING("opcua-esp32");

    UA_ServerConfig_setUriName(config, appUri, "OPC_UA_Server_ESP32");
    UA_ServerConfig_setCustomHostname(config, hostName);

    config->accessControl.clear(&config->accessControl);
    config->shutdownDelay = 5000.0;
    logins[1].username = UA_STRING("songa");
	logins[1].password = UA_STRING("songa");
    UA_StatusCode retval = UA_AccessControl_default(config, false,
        &config->securityPolicies[config->securityPoliciesSize - 1].policyUri, 2, logins);
	
	int nodeId = addNodeObject(server, UA_NS0ID_OBJECTSFOLDER, "ESP32", "NameSpace");
	nodeId = addNodeObject(server, UA_NS0ID_OBJECTSFOLDER, "Printer", "object");	
	int childid = addVariable(server, nodeId, "Firmware", UA_TYPES_STRING, true);
	WriteStringVariable(server, childid, "ESP32SC01");
	NodeidHeartbeat = addVariable(server, nodeId, "HeartBeat", UA_TYPES_INT32, true);
	childid = addVariable(server, nodeId, "X", UA_TYPES_FLOAT, true); 
	WriteFloatVariable(server, childid, 375.152f);
	childid = addVariable(server, nodeId, "Y", UA_TYPES_FLOAT, true);
	WriteFloatVariable(server, childid, 1375.152f);	
	childid = addVariable(server, nodeId, "Z", UA_TYPES_FLOAT, true);
	WriteFloatVariable(server, childid, 75.152f);

	retval = UA_Server_run_startup(server);
    g_bRunning = true;
	int OpcHeartBeat = 0;
    if (retval == UA_STATUSCODE_GOOD)
    {
        while (g_bRunning) {
            UA_Server_run_iterate(server, false);
            vTaskDelay(100 / portTICK_PERIOD_MS);
	        WriteIntVariable(server, NodeidHeartbeat , OpcHeartBeat);
	        OpcHeartBeat++;
        }
    }
    if (!g_bRunning)
        UA_Server_run_shutdown(server);
    g_bRunning = false;
}

void opc_task() {
    // pthread_t thread_id;
    // pthread_create(&thread_id, NULL, &ThreadRunOpcFunc, NULL);

    xTaskCreatePinnedToCore(thread_opc_task, "opcua_task", 24336, NULL, 10, NULL, 1);
}


#endif