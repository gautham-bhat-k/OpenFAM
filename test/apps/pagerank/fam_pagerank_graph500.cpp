/*
 * fam_pagerank_graph500.cpp
 * Copyright (c) 2019 Hewlett Packard Enterprise Development, LP. All rights
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
#include "common/fam_options.h"
#include "common/fam_test_config.h"
#include "pagerank_common.h"
#include <fam/fam.h>
#include <fam/fam_exception.h>
#include <iostream>
#include <fstream>
#include <omp.h>
#include <chrono>

using namespace std;
using namespace openfam;
using namespace chrono;

#define fam_stream cout << "PE(" << myPE << "/" << numPEs << ") "
//
// Enable PRINT_OUTPUT to save pagerank output to OUTFILE.
// It can be compared to pagerank non fam version for correctness.
//
//#define PRINT_OUTPUT 1
//#define OUTFILE "pagerank_fam.out"

uint64_t get_time() {
        long int time = static_cast<long int>(
            duration_cast<nanoseconds>(
                high_resolution_clock::now().time_since_epoch())
                .count());
        return time;
}

//#define GET_TIME() get_time()
#define GET_TIME() omp_get_wtime()

struct stats_time
{
        double  start_t,                        //start time
                stop_t,                         //stop time
                max_t,                          //maximum
                min_t,                          //minimum
                avg_t,                          //average time
                time;                           //total time
};
typedef struct stats_time _time;

int main(int argc, char **argv) {
    // Fam_Region_Descriptor *region = NULL;
    char configFilename[32];
    int matRowCount;
    int numSegments;
    char inpPath[PATH_MAX];
    char inpFilename[PATH_MAX];
    char tmpFilename[PATH_MAX];

    int each_pe_share = 0;
    Fam_Descriptor *rowDataItem[MAX_SEGMENTS];
    Fam_Descriptor *colDataItem[MAX_SEGMENTS];
    Fam_Descriptor *valDataItem[MAX_SEGMENTS];
    Fam_Descriptor *resultDataItem[MAX_SEGMENTS];
    Fam_Descriptor *inputVecDataItem;
    Fam_Descriptor *headerDataItem;
    const char *vector_dataitem_name[2] = { "Vector0", "Vector1" };
    int64_t nlocalverts[MAX_SEGMENTS];
    int growPerItr = 1024;
    int pagerank_Iter = 1;
    double *vector_in, *vector_out;
    // int64_t nlocaledges[MAX_SEGMENTS];
    int ret;

        _time   SpMV_    = {0,0,0,0,0,0},       //time taken to complete SpMV (communication + compute)
            file_    = {0,0,0,0,0,0},       //time taken to read the data from the file system
            out_     = {0,0,0,0,0,0},       //time taken to do the communication (put + get)
            compute_ = {0,0,0,0,0,0};       //time taken to complete the computation
    double  c_time_per_nnz = 0;         //compute time per nnz

    // FAM initialize
    // Open Preloaded region
    // region = spmv_fam_initialize();
    (void)spmv_fam_initialize();

    if (argc != 4) {
        if (myPE == 0) {
            cout << "usage" << endl;
            cout << argv[0] << " config name <rows per iter> <pagerank_Iter>" << endl;
        }
        exit(1);
    }
    strcpy(configFilename, argv[1]);
    spmv_read_config_data(configFilename, matRowCount, numSegments, inpPath);
    if (myPE == 0) {
        fam_stream << "Matrix : " << matRowCount << " X " << matRowCount
                   << endl;
        fam_stream << "Num Segments : " << numSegments << endl;
        fam_stream << "Data Path : " << inpPath << endl;
    }

    growPerItr = atoi(argv[2]);
    pagerank_Iter = atoi(argv[3]);
    //int outputSegments = atoi(argv[4]);
    sprintf(tmpFilename, "/item_names");
    strcpy(inpFilename, inpPath);
    strcat(inpFilename, tmpFilename);
    FILE *item_name_fp = fopen(inpFilename, "r");
    if (item_name_fp == NULL) {
        printf("File not found:%s\n", inpFilename);
        return -1;
    }
// Initialize row, coloumn and weight dataitems
for (int i = 0; i < numSegments; i = i + 1) {
    //char dataitemName[PATH_MAX];
    char *dataitemName = (char *)malloc(DATAITEM_NAME_LEN);
    int row;
    int64_t col, val;
    //file_.start_t = omp_get_wtime();
    //sprintf(dataitemName, "rowptr%d", i);
    fread(dataitemName, 1, DATAITEM_NAME_LEN, item_name_fp);
    //cout << "rowDataItem --> " << dataitemName << endl;
    rowDataItem[i] = spmv_get_dataitem(dataitemName);
    ret = spmv_fam_read((char *)&row, rowDataItem[i],
                            0, sizeof(int));
    if (ret < 0) {
    fam_stream << "ROW : fam wramup read failed" << endl;
    exit(1);
        }
    //sprintf(dataitemName, "colptr%d", i);
    fread(dataitemName, 1, DATAITEM_NAME_LEN, item_name_fp);
    //cout << "colDataItem --> " << dataitemName << endl;
    colDataItem[i] = spmv_get_dataitem(dataitemName);
    ret = spmv_fam_read((char *)&col, colDataItem[i],
                            0, sizeof(int64_t));
    if (ret < 0) {
    fam_stream << "COL : fam wramup read failed" << endl;
    exit(1);
        }
    //sprintf(dataitemName, "valptr%d", i);
    fread(dataitemName, 1, DATAITEM_NAME_LEN, item_name_fp);
    //cout << "valDataItem --> " << dataitemName << endl;
    valDataItem[i] = spmv_get_dataitem(dataitemName);
    ret = spmv_fam_read((char *)&val, valDataItem[i],
                            0, sizeof(int64_t));
    if (ret < 0) {
    fam_stream << "VAL : fam wramup read failed" << endl;
    exit(1);
        }
    free(dataitemName);
    //sprintf(dataitemName, "res_vector%d", i);
    //resultDataItem[i] = spmv_get_dataitem(dataitemName);
    //file_.stop_t = omp_get_wtime();
    //file_.time += file_.stop_t - file_.start_t;
}

for (int i = 0; i < numSegments; i = i + 1) {
        //char dataitemName[PATH_MAX];
        char *dataitemName = (char *)malloc(DATAITEM_NAME_LEN);
        double res=1.0;
        fread(dataitemName, 1, DATAITEM_NAME_LEN, item_name_fp);
        //cout << "resultDataItem --> " << dataitemName << endl;
        resultDataItem[i] = spmv_get_dataitem(dataitemName);
        ret = spmv_fam_write((char *)&res, resultDataItem[i],
                            0, sizeof(double));
        if (ret < 0) {
        fam_stream << "RES : fam wramup read failed" << endl;
        exit(1);
        }
        free(dataitemName);
}

char *headerDataitemName = (char *)malloc(DATAITEM_NAME_LEN);
 fread(headerDataitemName, 1, DATAITEM_NAME_LEN, item_name_fp);
    fclose(item_name_fp);
    inputVecDataItem = spmv_get_dataitem(vector_dataitem_name[0]);

    vector_in = (double *)spmv_malloc(sizeof(double) * matRowCount);
    if (vector_in == NULL) {
        fam_stream << "Input Vector alloc failed" << endl;
        exit(1);
    }

    ret = spmv_fam_read((char *)vector_in, inputVecDataItem,
                              0, sizeof(double) * matRowCount);
        if (ret < 0) {
            fam_stream << "Reading vector data failed" << endl;
            exit(1);
        }

#ifdef FAM_PROFILE
    my_fam->fam_reset_profile();
#endif
    headerDataItem = spmv_get_dataitem(headerDataitemName);
    free(headerDataitemName);
    if (myPE == 0) {
        char buf[1024] = { 0 };
        int ret = spmv_fam_write((char *)&buf, headerDataItem, 0, 1024);
        if (ret < 0) {
            fam_stream << "fam write failed to update header" << endl;
            exit(1);
        }
        fam_stream << "Global header initialized !!!" << endl;
    }
#if 0
    // initialize vector_in local vector
    vector_in = (double *)spmv_malloc(sizeof(double) * matRowCount);
    if (vector_in == NULL) {
        fam_stream << "Input Vector alloc failed" << endl;
        exit(1);
    }
#endif
    // initialize vector_out local vector
    vector_out = (double *)spmv_malloc(sizeof(double) * matRowCount);
    if (vector_out == NULL) {
        fam_stream << "Output vector alloc failed" << endl;
        exit(1);
    }
    //pagerank_initialize_vector (matRowCount, vector_dataitem_name[0]);

    for (int iter = 0; iter < pagerank_Iter; iter++) {
        // Read input vector, Vector0
        //ret = spmv_fam_read((char *)vector_in, vector_dataitem_name[iter % 2],
        //                    0, sizeof(double) * matRowCount);
#if 0
        ret = spmv_fam_read((char *)vector_in, inputVecDataItem,
                              0, sizeof(double) * matRowCount);
        if (ret < 0) {
            fam_stream << "Reading vector data failed" << endl;
            exit(1);
        }
        // Set the result Dataitem
        //out_.start_t = omp_get_wtime();
        //resultDataItem =
        //    spmv_get_dataitem(vector_dataitem_name[(iter + 1) % 2]);
        //out_.stop_t = omp_get_wtime();
        //out_.time += out_.stop_t - out_.start_t;

        if (myPE == 0)
            fam_stream << "Reading input  vector, pagerank iter = " << iter
                       << endl;
#endif

        {
            uint64_t size;
            // Allocate local buffers for row, col, val
            size = (MAX_ROW_COUNT + 5) * sizeof(int);
            grow_off = (int *)spmv_malloc(size);
            size = (MAX_NZ_COUNT) * sizeof(int64_t);
            gcol = (int64_t *)spmv_malloc(size);
            size = (MAX_NZ_COUNT) * sizeof(int64_t);
            gval = (float *)spmv_malloc(size);
        }
        for (int i = 0; i < numSegments; i++) {
            int64_t buf[2] = { 0 };

            // Load segment header
            ret = spmv_fam_read((char *)&buf, rowDataItem[i], 0, sizeof(buf));
            if (ret < 0) {
                fam_stream << "Reading row data failed" << endl;
                exit(1);
            }
            nlocalverts[i] = buf[0];
	    //cout << "number of elements per seg : " << nlocalverts[i] << endl;
        }
        my_fam->fam_barrier_all();
        SpMV_.start_t = GET_TIME();
        int gRowCorrection = 0;
        int gRowsPerSeg = (int)nlocalverts[0] - 1;
        int grow;
        while ((grow = spmv_get_next_row(headerDataItem, 0, growPerItr)) <
               matRowCount) {
            //fam_stream << "Multiplying row  " << grow << " " << growPerItr
            //           << " iter = " << iter << endl;
            int growEnd = grow + growPerItr;
            if (growEnd > matRowCount)
                growEnd = matRowCount;

            for (; grow < (growEnd);
                grow = grow + growPerItr - gRowCorrection) {
                int ret;
                uint64_t rowOff = (2 * sizeof(int64_t));
                uint64_t colOff = 0;
                uint64_t valOff = 0;
                uint64_t rowSize;
                uint64_t colSize;
                uint64_t valSize;
                int gtotalRows;
                int gSegRowIdx;
                int segIdx;
                int colEntries;

                gRowCorrection = 0;
                segIdx = grow / gRowsPerSeg;
		//cout << "PE : " << myPE << " segIdx : " << segIdx << endl;
                gSegRowIdx = grow % gRowsPerSeg;
                rowOff += (gSegRowIdx) * sizeof(int);
                                file_.start_t = GET_TIME();
                ret = spmv_fam_read((char *)&colEntries, rowDataItem[segIdx],
                                    rowOff, sizeof(int));
                                file_.stop_t = GET_TIME();
                file_.time += file_.stop_t - file_.start_t;

                if (ret < 0) {
                    fam_stream << "Reading  &colEntriesdata failed" << endl;
                    exit(1);
                }
                colOff = colEntries * sizeof(int64_t);
                valOff = colEntries * sizeof(float);
                gtotalRows = growPerItr;
                if (gSegRowIdx + gtotalRows > nlocalverts[segIdx]) {
                    int totalRows = (int)nlocalverts[segIdx] - gSegRowIdx;
                    gRowCorrection = gtotalRows - totalRows;
                    gtotalRows -= gRowCorrection;
                }
                {
                    int entries;

                    if ((gSegRowIdx + gtotalRows) >= nlocalverts[segIdx]) {
                        gtotalRows = (int)nlocalverts[segIdx] - gSegRowIdx - 1;
                    }
                    // Load row into grow_off
                    rowSize = (gtotalRows + 1) * sizeof(int);
                                        file_.start_t = GET_TIME();
                    ret = spmv_fam_read((char *)grow_off, rowDataItem[segIdx],
                                        rowOff, rowSize);
                                        file_.stop_t = GET_TIME();
                        file_.time += file_.stop_t - file_.start_t;
                    if (ret < 0) {
                        fam_stream << "Reading row data failed" << endl;
                        exit(1);
                    }
                    entries = grow_off[gtotalRows] - grow_off[0];
		    //printf("PE : %d, segment : %d, seg row id : %d, end : %d, start : %d\n", myPE, segIdx, gSegRowIdx,  grow_off[gtotalRows], grow_off[0]);
                    if (!entries) {
                        continue;
                    }

                    // Load column into gcol
                    colSize = entries * sizeof(int64_t);
                                        file_.start_t = GET_TIME();
                    ret = spmv_fam_read((char *)gcol, colDataItem[segIdx],
                                        colOff, colSize);
                                        file_.stop_t = GET_TIME();
                        file_.time += file_.stop_t - file_.start_t;
                    if (ret < 0) {
                        fam_stream << "Reading column data failed" << endl;
                        exit(1);
                    }

                    // Load weight into gval
                    valSize = entries * sizeof(float);
                                        file_.start_t = GET_TIME();
                    ret = spmv_fam_read((char *)gval, valDataItem[segIdx],
                                        valOff, valSize);
                                        file_.stop_t = GET_TIME();
                        file_.time += file_.stop_t - file_.start_t;
                    if (ret < 0) {
                        fam_stream << "Reading weight data failed" << endl;
                        exit(1);
                    }

                    // Multiply
                                        compute_.start_t = GET_TIME();
                    spmv_compute(0, gtotalRows, vector_in, vector_out);
                                        compute_.stop_t = GET_TIME();
                    compute_.time += compute_.stop_t - compute_.start_t;
		    each_pe_share++;
                    // Write output vector
                                        out_.start_t = GET_TIME();
                    //ret = spmv_fam_write((char *)vector_out, resultDataItem[segIdx],
                    //                     ((segIdx * gRowsPerSeg) + gSegRowIdx) *
                    //                         sizeof(double),
                    //                     sizeof(double) * gtotalRows);
                    ret = spmv_fam_write((char *)vector_out, resultDataItem[segIdx], gSegRowIdx*sizeof(double), sizeof(double) * gtotalRows);
                                        out_.stop_t = GET_TIME();
                    out_.time += out_.stop_t - out_.start_t;
                    //fam_stream << "out_.time : " << (out_.stop_t - out_.start_t)*1e6 << endl;
                    if (ret < 0) {
                        fam_stream << "fam write failed to update result"
                                   << endl;
                    }
                }
            }
        }

    SpMV_.stop_t = GET_TIME();
    SpMV_.time = SpMV_.stop_t - SpMV_.start_t;
        my_fam->fam_barrier_all();
        if (myPE == 0) {
            // For the next iteration read rows from the beginning
            spmv_set_row_zero(headerDataItem);
            //fam_stream << "Completed !!!" << endl;
        }
        my_fam->fam_barrier_all();
	cout << "Final grow : " << grow << endl;
    }
#ifdef PRINT_OUTPUT
    if (myPE == 0) {
        ofstream outfile;
        outfile.open (OUTFILE);

        ret = spmv_fam_read((char *)vector_in, vector_dataitem_name[0],
                            0, sizeof(double) * matRowCount);
        if (ret < 0) {
            fam_stream << "Reading vector data failed" << endl;
            exit(1);
        }
        ret = spmv_fam_read((char *)vector_out, vector_dataitem_name[1],
                            0, sizeof(double) * matRowCount);
        if (ret < 0) {
            fam_stream << "Reading vector data failed" << endl;
            exit(1);
        }

        for (int i = 0;i<matRowCount;i++) {
            outfile<<"Row : "<<i<<" Vector0 : "<<vector_in[i]<<" Vector1 : "<<vector_out[i]<<std::endl;
        }
        outfile.close();

    }
#endif

    //c_time_per_nnz = compute_.time*1e9/prev_pe_elems;
    //printf("PE: %d --> SpMV: %.2f ms\tfile: %.2f ms\tcompute: %.2f ms\toutput write: %.2f ms\n",myPE, SpMV_.time*1e3, file_.time*1e3, compute_.time*1e3, out_.time*1e3);
    //printf("%d,%d,%lu,%lu,%lu,%lu\n",numPEs,myPE, SpMV_.time, file_.time, compute_.time, out_.time);
    printf("%d,%d,%.2f,%.2f,%.2f,%.2f,%d\n",numPEs,myPE, SpMV_.time*1e3, file_.time*1e3, compute_.time*1e3, out_.time*1e3, each_pe_share);
    my_fam->fam_finalize("default");
    free(grow_off);
    free(gcol);
    free(gval);
    delete my_fam;
    return 0;
}

