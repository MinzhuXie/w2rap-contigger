///////////////////////////////////////////////////////////////////////////////
//                   SOFTWARE COPYRIGHT NOTICE AGREEMENT                     //
//       This software and its documentation are copyright (2014) by the     //
//   Broad Institute.  All rights are reserved.  This software is supplied   //
//   without any warranty or guaranteed support whatsoever. The Broad        //
//   Institute is not responsible for its use, misuse, or functionality.     //
///////////////////////////////////////////////////////////////////////////////

#ifndef LARGE_REPATH_H
#define LARGE_REPATH_H

#include "Basevector.h"
#include "CoreTools.h"
#include "paths/HyperBasevector.h"
#include "paths/long/ReadPath.h"


void RepathInMemory( const HyperBasevector& hb, const vecbasevector& edges,
                const vec<int>& inv, ReadPathVec& paths, const int K, const int K2,
                HyperBasevector& hb2, ReadPathVec& paths2 , const Bool REPATH_TRANSLATE, bool INVERT_PATHS,
                const Bool EXTEND_PATHS );

void RepathInMemoryEXP( const HyperBasevector& old_hbv, const vec<int>& old_hbvinv, ReadPathVec& old_paths,
                        const int new_K, HyperBasevector& new_hbv, ReadPathVec& new_paths );

#endif
