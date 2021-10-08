#include "metadata_service/fam_metadata_mercury_rpc.h"

namespace openfam {
Fam_Metadata_Service_Direct *Fam_Metadata_Mercury_RPC::metadataService = new Fam_Metadata_Service_Direct();
Fam_Metadata_Mercury_RPC::Fam_Metadata_Mercury_RPC() {
	//Fam_Metadata_Mercury_RPC::metadataService = new Fam_Metadata_Service_Direct();
}

Fam_Metadata_Service_Direct *Fam_Metadata_Mercury_RPC::get_metadata_service() {
	return Fam_Metadata_Mercury_RPC::metadataService;
}

hg_id_t Fam_Metadata_Mercury_RPC::register_with_mercury(hg_class_t *hg_class)
{
    //hg_class_t *hg_class;
    hg_id_t tmp;

    if(!hg_class) {
    	hg_class = hg_engine_get_class();
	cout << "HG class is empty hence creating new" << endl;
    }

    tmp = MERCURY_REGISTER(
        hg_class, "metadata_lookup_item", my_rpc_in_t, my_rpc_out_t, Fam_Metadata_Mercury_RPC::fam_metadata_merc_lookup);

    return (tmp);
}

hg_return_t Fam_Metadata_Mercury_RPC::fam_metadata_merc_lookup(hg_handle_t handle) {
                my_rpc_in_t request;
                //request = (Fam_Metadata_Merc_Request *)malloc(sizeof(Fam_Metadata_Merc_Request));

                hg_return_t ret;

                ret = HG_Get_input(handle, &request);
                assert(ret == HG_SUCCESS);

                my_rpc_out_t response;
		//bool result;
                Fam_DataItem_Metadata dataitem;
		try {
                	Fam_Metadata_Mercury_RPC::metadataService->metadata_find_dataitem_and_check_permissions((metadata_region_item_op_t)request.op, request.key_dataitem_name, request.key_region_name, request.uid, request.gid, dataitem);
		} catch(Fam_Exception &e) {
			//response.isfound = false;	
			response.errorcode = e.fam_error();
			response.errormsg = strdup(e.fam_error_msg());
		}
                response.region_id = dataitem.regionId;	
                response.name = dataitem.name;
                response.offset = dataitem.offset;
                response.size = dataitem.size;
                response.perm = dataitem.perm;
                response.uid = dataitem.uid;
                response.gid = dataitem.gid;
                response.maxkeylen = Fam_Metadata_Mercury_RPC::metadataService->metadata_maxkeylen();
                response.memsrv_id = dataitem.memoryServerId;
		//response.isfound = result;
		response.errorcode = 0;
		response.errormsg = strdup("");

		//cout << "Original::data item name : " << dataitem.name << " Region Id : " << dataitem.regionId << " Offset : " << dataitem.offset << endl;
		//cout << "Response::data item name : " << response.name << " Region Id : " << response.region_id << " Offset : " << response.offset << endl;
                ret = HG_Respond(handle, NULL, NULL, &response);
                assert(ret == HG_SUCCESS);
                (void) ret;
		HG_Destroy(handle);
                return ret;
}
}
