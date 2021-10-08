#ifndef FAM_METADATA_MERCURY_H
#define FAM_METADATA_MERCURY_H


#include "common/mercury_engine.h"

#include "common/fam_internal.h"
//#include "metadata_service/fam_metadata_service_direct.h"

using namespace metadata;
#if 0
struct Fam_Metadata_Merc_Request {
    uint64_t key_region_id;
    uint64_t key_dataitem_id;
    string key_region_name;
    string key_dataitem_name;
    uint64_t region_id;
    string name;
    uint64_t offset;
    uint32_t uid;
    uint32_t gid;
    uint64_t perm;
    uint64_t size;
    uint32_t user_policy;
    uint64_t memsrv_id;
    int32_t op;
};

struct Fam_Metadata_Merc_Response {
    uint64_t region_id;
    string name;
    uint64_t offset;
    uint32_t uid;
    uint32_t gid;
    uint64_t perm;
    uint64_t size;
    uint64_t maxkeylen;
    bool isfound;
    int32_t errorcode;
    string errormsg;
    uint64_t memsrv_id;
};
#endif
namespace openfam {
typedef struct {
    bool done;
    bool isFound;
    size_t maxKeyLen;
    Fam_Region_Item_Info itemInfo;
} Merc_RPC_State;

//Fam_Metadata_Service_Direct *metadataService_;

MERCURY_GEN_PROC(my_rpc_in_t, ((uint64_t)(key_region_id))((uint64_t)(key_dataitem_id))((hg_const_string_t)(key_region_name))((hg_const_string_t)(key_dataitem_name))((uint64_t)(region_id))((uint64_t)(offset))((uint32_t)(uid))((uint32_t)(gid))((uint64_t)(perm))((uint64_t)(size))((uint32_t)(user_policy))((uint64_t)(memsrv_id))((int32_t)(op)))
MERCURY_GEN_PROC(my_rpc_out_t, ((uint64_t)(region_id))((hg_const_string_t)(name))((uint64_t)(offset))((uint32_t)(uid))((uint32_t)(gid))((uint64_t)(perm))((uint64_t)(size))((uint64_t)(maxkeylen))((hg_bool_t)(isfound))((int32_t)(errorcode))((hg_const_string_t)(errormsg))((uint64_t)(memsrv_id)));

hg_id_t
register_with_mercury(void);
#if 0
static hg_return_t fam_metadata_merc_lookup(hg_handle_t handle) {
                my_rpc_in_t request;
                //request = (Fam_Metadata_Merc_Request *)malloc(sizeof(Fam_Metadata_Merc_Request));

                hg_return_t ret;

                ret = HG_Get_input(handle, &request);
                assert(ret == HG_SUCCESS);

                Fam_DataItem_Metadata dataitem;
                bool result = metadataService_->metadata_find_dataitem(request.key_dataitem_name, request.key_region_name, dataitem);

                my_rpc_out_t response;
                response.region_id = dataitem.regionId;
                response.name = dataitem.name;
                response.offset = dataitem.offset;
                response.size = dataitem.size;
                response.perm = dataitem.perm;
                response.uid = dataitem.uid;
                response.gid = dataitem.gid;
                response.maxkeylen = metadataService_->metadata_maxkeylen();
                response.memsrv_id = dataitem.memoryServerId;

                ret = HG_Respond(handle, NULL, NULL, &response);
                assert(ret == HG_SUCCESS);
                (void) ret;

                return ret;
}

class Fam_Metadata_Merc {
   public:
	Fam_Metadata_Merc(Fam_Metadata_Service_Direct *metadataService_) {
		metadataService = metadataService_;
	}
	~Fam_Metadata_Merc(){}

	static hg_return_t fam_metadata_merc_lookup(hg_handle_t handle) {
		my_rpc_in_t request;
		//request = (Fam_Metadata_Merc_Request *)malloc(sizeof(Fam_Metadata_Merc_Request));

		hg_return_t ret;

		ret = HG_Get_input(handle, &request);
		assert(ret == HG_SUCCESS);

		Fam_DataItem_Metadata dataitem;
		bool result = metadataService->metadata_find_dataitem(request.key_dataitem_name, request.key_region_name, dataitem);	

		my_rpc_out_t response;	
		response.region_id = dataitem.regionId;
		response.name = dataitem.name;
		response.offset = dataitem.offset;
		response.size = dataitem.size;
		response.perm = dataitem.perm;
		response.uid = dataitem.uid;
		response.gid = dataitem.gid;
		response.maxkeylen = metadataService->metadata_maxkeylen();
		response.memsrv_id = dataitem.memoryServerId;

		ret = HG_Respond(handle, NULL, NULL, &response);
		assert(ret == HG_SUCCESS);
		(void) ret;
		
		return ret;
	}

   private:
	static Fam_Metadata_Service_Direct *metadataService;
};
#endif
}
#endif
