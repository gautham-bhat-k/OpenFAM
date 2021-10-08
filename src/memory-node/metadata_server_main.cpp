/*
 * metadata_server_main.cpp
 * Copyright (c) 2020 Hewlett Packard Enterprise Development, LP. All rights
 * reserved. Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * See https://spdx.org/licenses/BSD-3-Clause
 *
 */

#ifdef COVERAGE
#include <signal.h>
#endif

#include "metadata_service/fam_metadata_service_server.h"
#include "metadata_service/fam_metadata_mercury_rpc.h"
#include "common/mercury_server_init.h"

#include <iostream>
using namespace std;
using namespace metadata;

#define HG_TEST_HAS_THREAD_POOL

#ifdef OPENFAM_VERSION
#define METADATASERVER_VERSION OPENFAM_VERSION
#else
#define METADATASERVER_VERSION "0.0.0"
#endif

#ifdef COVERAGE
extern "C" void __gcov_flush();
void signal_handler(int signum) {
    cout << "Shutting down metadata server!! signal #" << signum << endl;
    __gcov_flush();
    exit(1);
}
#endif

#if 0
Fam_Metadata_Service_Direct *metadataDirect;

static hg_return_t fam_metadata_merc_lookup(hg_handle_t handle) {
                my_rpc_in_t request;
                //request = (Fam_Metadata_Merc_Request *)malloc(sizeof(Fam_Metadata_Merc_Request));

                hg_return_t ret;

                ret = HG_Get_input(handle, &request);
                assert(ret == HG_SUCCESS);

                Fam_DataItem_Metadata dataitem;
                bool result = metadataDirect->metadata_find_dataitem(request.key_dataitem_name, request.key_region_name, dataitem);

                my_rpc_out_t response;
                response.region_id = dataitem.regionId;
                response.name = dataitem.name;
                response.offset = dataitem.offset;
                response.size = dataitem.size;
                response.perm = dataitem.perm;
                response.uid = dataitem.uid;
                response.gid = dataitem.gid;
                response.maxkeylen = metadataDirect->metadata_maxkeylen();
                response.memsrv_id = dataitem.memoryServerId;

                ret = HG_Respond(handle, NULL, NULL, &response);
                assert(ret == HG_SUCCESS);
                (void) ret;

                return ret;
}
#endif

Fam_Metadata_Service_Server *metadataService;
Fam_Metadata_Service_Direct *direct;
Fam_Metadata_Mercury_RPC *metadataMercServer;
int main(int argc, char *argv[]) {
    uint64_t rpcPort = 8788;
    char *name = strdup("127.0.0.1");

    for (int i = 1; i < argc; i++) {
        if ((std::string(argv[i]) == "-v") ||
            (std::string(argv[i]) == "--version")) {
            cout << "Metadata Server version : " << METADATASERVER_VERSION
                 << "\n";
            exit(0);
        } else if ((std::string(argv[i]) == "-h") ||
                   (std::string(argv[i]) == "--help")) {
            cout
                << "Usage : \n"
                << "\t./metadataserver <options> \n"
                << "\n"
                << "Options : \n"
                << "\t-a/--address   : Address of the metadata server "
                   "(default value is localhost) \n"
                << "\n"
                << "\t-r/--rpcport        : RPC port (default value is 8787)\n"
                << "\n"
                << "\t-v/--version        : Display metadata server version  \n"
                << "\n"
                << endl;
            exit(0);
        } else if ((std::string(argv[i]) == "-a") ||
                   (std::string(argv[i]) == "--address")) {
            name = strdup(argv[++i]);
        } else if ((std::string(argv[i]) == "-r") ||
                   (std::string(argv[i]) == "--rpcport")) {
            rpcPort = atoi(argv[++i]);
        } else 
	    continue;
    }
    int argc_tmp = argc-4;
    char *argv_tmp[50];
    int j=1;
    argv_tmp[0] = argv[0];
    for(int i=5; i<argc; i++) {
	argv_tmp[j] = argv[i];
	j++;
    } 
    
#ifdef COVERAGE
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGTERM, signal_handler);
#endif

    metadataService = NULL;
    try {
#ifdef USE_MERCURY
	metadataMercServer = new Fam_Metadata_Mercury_RPC();
	direct = metadataMercServer->get_metadata_service();
	cout << "name : " << name << " port : " << rpcPort << endl;
        metadataService = new Fam_Metadata_Service_Server(rpcPort, name, direct);
	//hg_engine_init(NA_TRUE, "ofi+psm2");
	//hg_engine_print_self_addr();
	struct hg_test_info init_info;
	mercury_server_init(argc_tmp, argv_tmp, init_info);
	cout << "Mercury server initialized...." << endl;
	if(!init_info.hg_class) {
		cout << "Init info is empty ...." << endl;
		exit(1);
	}
	metadataMercServer->register_with_mercury(init_info.hg_class);
	cout << "Mercury reg done..." << endl;
        metadataService->run();
#else
	metadataService = new Fam_Metadata_Service_Server(rpcPort, name);
	metadataService->run();
#endif
    } catch (Fam_Exception &e) {
        if (metadataService) {
            delete metadataService;
        }
        cout << "Error code: " << e.fam_error() << endl;
        cout << "Error msg: " << e.fam_error_msg() << endl;
    }

    if (metadataService) {
        delete metadataService;
        metadataService = NULL;
    }
    if (metadataMercServer) {
	delete metadataMercServer;
	metadataMercServer = NULL;
	hg_engine_finalize();
    }

    return 0;
}
