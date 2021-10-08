#include "metadata_service/fam_metadata_mercury.h"
#include "metadata_service/fam_metadata_mercury_server.h"
namespace openfam {

//static hg_return_t fam_metadata_merc_lookup(hg_handle_t handle);

//Fam_Metadata_Service_Direct *metadataService_;

hg_id_t
register_with_mercury(void)
{
    hg_class_t *hg_class;
    hg_id_t tmp;

    hg_class = hg_engine_get_class();

    tmp = MERCURY_REGISTER(
        hg_class, "metadata_lookup_item", my_rpc_in_t, my_rpc_out_t, Fam_Metadata_Mercury_Server::fam_metadata_merc_lookup);

    return (tmp);
}

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
#endif
}
