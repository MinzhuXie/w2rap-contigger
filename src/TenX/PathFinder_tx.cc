//
// Created by Bernardo Clavijo (TGAC) on 11/07/2016.
//

#include "TenX/PathFinder_tx.h"

PathFinder_tx::PathFinder_tx (TenXPather* txp, HyperBasevector* hbv, vec<int> inv, int min_reads = 5) {
    mHBV = hbv;
    mInv = inv;
    mMinReads = min_reads;
    mTxp = txp;

    hbv->ToLeft(mToLeft);
    hbv->ToRight(mToRight);
}

std::string PathFinder_tx::edge_pstr(uint64_t e){
    return "e"+std::to_string(e)+"("+std::to_string(mHBV->EdgeObject(e).size())+"bp "+std::to_string(paths_per_kbp(e))+"ppk)";
}

void PathFinder_tx::init_prev_next_vectors(){
    //TODO: this is stupid duplication of the digraph class, but it's so weird!!!
    prev_edges.resize(mToLeft.size());
    next_edges.resize(mToRight.size());
    for (auto e=0;e<mToLeft.size();++e){

        uint64_t prev_node=mToLeft[e];

        prev_edges[e].resize(mHBV->ToSize(prev_node));
        for (int i=0;i<mHBV->ToSize(prev_node);++i){
            prev_edges[e][i]=mHBV->EdgeObjectIndexByIndexTo(prev_node,i);
        }

        uint64_t next_node=mToRight[e];

        next_edges[e].resize(mHBV->FromSize(next_node));
        for (int i=0;i<mHBV->FromSize(next_node);++i){
            next_edges[e][i]=mHBV->EdgeObjectIndexByIndexFrom(next_node,i);
        }
    }


}

//std::array<uint64_t,3> PathFinder_tx::transition_votes(uint64_t left_e,uint64_t right_e){
//    std::array<uint64_t,3> tv={0,0,0};
//    std::cout<<"Scoring transition from "<<left_e<<"to"<<right_e<<std::endl;
//
//    std::cout<<"Paths on right edge "<<mEdgeToPathIds[right_e].size()<<" inv "<<mEdgeToPathIds[mInv[right_e]].size()<<std::endl;
//    return tv;
//};

std::array<uint64_t,3> PathFinder_tx::path_votes(std::vector<uint64_t> path){
    //Returns a vote vector: { FOR, PARTIAL (reads end halfway, but validate at least one transition), AGAINST }
    //does this on forward and reverse paths, just in case
    std::vector<ReadPath> vfor,vpartial,vagainst;
    //TODO: needs to be done in both directions? (votes for only in one direction?)

    //std::cout<<std::endl<<"scoring path!!!!"<<std::endl;
    //first detect paths going out of first edge, also add them to open_paths
    std::list<std::pair<ReadPath,uint16_t>> initial_paths, open_paths;
    //std::cout<<"starting at edge "<<path[0]<<std::endl;
    for (auto pi:mEdgeToPathIds[path[0]]){

        auto p=mPaths[pi];
        //std::cout<<p<<std::endl;
        if (p.size()>1){
            uint16_t i=0;
            while (p[i]!=path[0]) ++i;
            if (i<p.size()-1) {
                open_paths.insert(open_paths.end(),std::make_pair(p,i));
                //std::cout<<" inserting into open paths"<<std::endl;
            }
        }
    }
    initial_paths.insert(initial_paths.begin(),open_paths.begin(),open_paths.end());
    //std::cout<<"initial paths generated from edge"<<path[0]<<" "<<initial_paths.size() <<std::endl;
    // basically every path in the mEdgeToPathIds[e] is either on the openPaths, starts here o
    for (auto ei=1;ei<path.size();++ei) {

        auto e=path[ei];
        //std::cout<<" going through edge "<<e<<" open list size: "<<open_paths.size()<<" paths on this edge: "<<mEdgeToPathIds[e].size()<<std::endl;
        //First go through the open list
        for (auto o=open_paths.begin();o!=open_paths.end();) {
            //if goes somewhere else, vote against and remove
            if (o->first[o->second+1]!=e){
                //std::cout<<"AGAINST: previous path goes to "<<o->first[o->second+1]<<std::endl;
                vagainst.push_back(o->first);
                o=open_paths.erase(o);
            } else { //else, advance
                ++(o->second);
                ++o;
            }
        }

        //std::cout<<" open list processed, votes" << pv[0]<<":"<<pv[1]<<":"<<pv[2]<<std::endl;

        std::list<std::pair<ReadPath,uint16_t>> new_paths;

        //check paths coming here from somewhere else and vote against
        for (auto ip:mEdgeToPathIds[e]) {
            //path is of size 1: irrelevant
            auto p=mPaths[ip];
            //std::cout<<"Considering path "<<ip<<":"<<p<<std::endl;
            if (p.size()==1) continue;

            //path starts_here, add to the open list later TODO: no need on the last edge!
            if (p[0]==e) {
                new_paths.insert(new_paths.end(),std::make_pair(p,0));
                //std::cout<<"adding new path to the open list"<<std::endl;
                continue;
            }

            //path in the open list?
            auto lp=open_paths.begin();
            for (;lp!=open_paths.end();++lp){
                if (p==lp->first) break;
            }

            if (lp!=open_paths.end()){
                //path is in the open list

                //last edge?
                if (ei==path.size()-1) {
                    //check if path in initial paths
                    auto ipp=initial_paths.begin();
                    for (;ipp!=initial_paths.end();++ipp){
                        if (p==ipp->first) break;
                    }
                    //path in initial_paths?
                    if (ipp!=initial_paths.end()){
                        //++pv[0];
                        vfor.push_back(p);
                        //std::cout<<"FOR: last edge and this path was in the initial list"<<std::endl;
                    }
                    else{
                        //++pv[1];
                        vpartial.push_back(p);
                        //std::cout<<"PARTIAL: last edge and this path was NOT in the initial list"<<std::endl;
                    }
                } else if (lp->first.size()-1==lp->second){
                    //++pv[1];
                    vpartial.push_back(p);
                    open_paths.erase(lp);
                    //std::cout<<"PARTIAL: path finished before the last edge"<<std::endl;
                }
            } else {
                //path comes from somewhere else, vote against
                //++pv[2];
                vagainst.push_back(p);
                //std::cout<<"AGAINST: path comes from somewhere else"<<std::endl;
            }

        }

        open_paths.insert(open_paths.end(),new_paths.begin(),new_paths.end());
    }

    //auto tv=transition_votes(path[i],path[i+1])
    //std::cout<<"Paths on left edge "<<mEdgeToPathIds[left_e].size()<<" inv "<<mEdgeToPathIds[mInv[left_e]].size()<<std::endl;
    std::array<uint64_t,3> pv={0,0,0};
    //collect votes:
    //on favour are on favour:
    //pv[0]=vfor.size();
    std::vector<ReadPath> votes_used;

    for (auto vf:vfor){
        bool u=false;
        for (auto vu:votes_used) if (vu.same_read(vf)) {u=true;break;}
        if (!u) {
            votes_used.insert(votes_used.end(), vf);
            pv[0]++;
        }
    }
    for (auto vp:vpartial){
        bool u=false;
        for (auto vu:votes_used) if (vu.same_read(vp)) {u=true;break;}
        if (!u) {
            votes_used.insert(votes_used.end(), vp);
            pv[1]++;
        }
    }
    for (auto va:vagainst){
        bool u=false;
        for (auto vu:votes_used) if (vu.same_read(va)) {u=true;break;}
        if (!u) {
            votes_used.insert(votes_used.end(), va);
            pv[2]++;
        }
    }
    return pv;
}

std::string PathFinder_tx::path_str(std::vector<uint64_t> path) {
    std::string s="[";
    for (auto p:path){
        s+=std::to_string(p)+":"+std::to_string(mInv[p])+" ";//+" ("+std::to_string(mHBV->EdgeObject(p).size())+"bp "+std::to_string(paths_per_kbp(p))+"ppk)  ";
    }
    s+="]";
    return s;
}

std::array<uint64_t,3> PathFinder_tx::multi_path_votes(std::vector<std::vector<uint64_t>> paths){
    //Returns a vote vector: { FOR, PARTIAL (reads end halfway, but validate at least one transition), AGAINST }
    //does this on forward and reverse paths, just in case
    std::vector<ReadPath> vfor,vpartial,vagainst;
    //TODO: needs to be done in both directions? (votes for only in one direction?)
    for (auto path:paths) {
        //std::cout<<std::endl<<"scoring path!!!!"<<std::endl;
        //first detect paths going out of first edge, also add them to open_paths
        std::list<std::pair<ReadPath, uint16_t>> initial_paths, open_paths;
        //std::cout<<"starting at edge "<<path[0]<<std::endl;
        for (auto pi:mEdgeToPathIds[path[0]]) {

            auto p = mPaths[pi];
            //std::cout<<p<<std::endl;
            if (p.size() > 1) {
                uint16_t i = 0;
                while (p[i] != path[0]) ++i;
                if (i < p.size() - 1) {
                    open_paths.insert(open_paths.end(), std::make_pair(p, i));
                    //std::cout<<" inserting into open paths"<<std::endl;
                }
            }
        }
        initial_paths.insert(initial_paths.begin(), open_paths.begin(), open_paths.end());
        //std::cout<<"initial paths generated from edge"<<path[0]<<" "<<initial_paths.size() <<std::endl;
        // basically every path in the mEdgeToPathIds[e] is either on the openPaths, starts here o
        for (auto ei = 1; ei < path.size(); ++ei) {

            auto e = path[ei];
            //std::cout<<" going through edge "<<e<<" open list size: "<<open_paths.size()<<" paths on this edge: "<<mEdgeToPathIds[e].size()<<std::endl;
            //First go through the open list
            for (auto o = open_paths.begin(); o != open_paths.end();) {
                //if goes somewhere else, vote against and remove
                if (o->first[o->second + 1] != e) {
                    //std::cout<<"AGAINST: previous path goes to "<<o->first[o->second+1]<<std::endl;
                    vagainst.push_back(o->first);
                    o = open_paths.erase(o);
                } else { //else, advance
                    ++(o->second);
                    ++o;
                }
            }

            //std::cout<<" open list processed, votes" << pv[0]<<":"<<pv[1]<<":"<<pv[2]<<std::endl;

            std::list<std::pair<ReadPath, uint16_t>> new_paths;

            //check paths coming here from somewhere else and vote against
            for (auto ip:mEdgeToPathIds[e]) {
                //path is of size 1: irrelevant
                auto p = mPaths[ip];
                //std::cout<<"Considering path "<<ip<<":"<<p<<std::endl;
                if (p.size() == 1) continue;

                //path starts_here, add to the open list later TODO: no need on the last edge!
                if (p[0] == e) {
                    new_paths.insert(new_paths.end(), std::make_pair(p, 0));
                    //std::cout<<"adding new path to the open list"<<std::endl;
                    continue;
                }

                //path in the open list?
                auto lp = open_paths.begin();
                for (; lp != open_paths.end(); ++lp) {
                    if (p == lp->first) break;
                }

                if (lp != open_paths.end()) {
                    //path is in the open list

                    //last edge?
                    if (ei == path.size() - 1) {
                        //check if path in initial paths
                        auto ipp = initial_paths.begin();
                        for (; ipp != initial_paths.end(); ++ipp) {
                            if (p == ipp->first) break;
                        }
                        //path in initial_paths?
                        if (ipp != initial_paths.end()) {
                            //++pv[0];
                            vfor.push_back(p);
                            //std::cout<<"FOR: last edge and this path was in the initial list"<<std::endl;
                        }
                        else {
                            //++pv[1];
                            vpartial.push_back(p);
                            //std::cout<<"PARTIAL: last edge and this path was NOT in the initial list"<<std::endl;
                        }
                    } else if (lp->first.size() - 1 == lp->second) {
                        //++pv[1];
                        vpartial.push_back(p);
                        open_paths.erase(lp);
                        //std::cout<<"PARTIAL: path finished before the last edge"<<std::endl;
                    }
                } else {
                    //path comes from somewhere else, vote against
                    //++pv[2];
                    vagainst.push_back(p);
                    //std::cout<<"AGAINST: path comes from somewhere else"<<std::endl;
                }

            }

            open_paths.insert(open_paths.end(), new_paths.begin(), new_paths.end());
        }
    }
    //auto tv=transition_votes(path[i],path[i+1])
    //std::cout<<"Paths on left edge "<<mEdgeToPathIds[left_e].size()<<" inv "<<mEdgeToPathIds[mInv[left_e]].size()<<std::endl;
    std::array<uint64_t,3> pv={0,0,0};
    //collect votes:
    //on favour are on favour:
    //pv[0]=vfor.size();
    std::vector<ReadPath> votes_used;

    for (auto vf:vfor){
        bool u=false;
        for (auto vu:votes_used) if (vu.same_read(vf)) {u=true;break;}
        if (!u) {
            votes_used.insert(votes_used.end(), vf);
            pv[0]++;
        }
    }
    for (auto vp:vpartial){
        bool u=false;
        for (auto vu:votes_used) if (vu.same_read(vp)) {u=true;break;}
        if (!u) {
            votes_used.insert(votes_used.end(), vp);
            pv[1]++;
        }
    }
    for (auto va:vagainst){
        bool u=false;
        for (auto vu:votes_used) if (vu.same_read(va)) {u=true;break;}
        if (!u) {
            votes_used.insert(votes_used.end(), va);
            pv[2]++;
        }
    }
    return pv;
}



void PathFinder_tx::classify_forks(){
    int64_t nothing_fw=0, line_fw=0,join_fw=0,split_fw=0,join_split_fw=0;
    uint64_t nothing_fw_size=0, line_fw_size=0,join_fw_size=0,split_fw_size=0,join_split_fw_size=0;
    for ( int i = 0; i < mHBV->EdgeObjectCount(); ++i ) {
        //checks for both an edge and its complement, because it only looks forward
        uint64_t out_node=mToRight[i];
        if (mHBV->FromSize(out_node)==0) {
            nothing_fw++;
            nothing_fw_size+=mHBV->EdgeObject(i).size();
        } else if (mHBV->FromSize(out_node)==1) {
            if (mHBV->ToSize(out_node)==1){
                line_fw++;
                line_fw_size+=mHBV->EdgeObject(i).size();
            } else {
                split_fw++;
                split_fw_size+=mHBV->EdgeObject(i).size();
            }
        } else if (mHBV->ToSize(out_node)==1){
            join_fw++;
            join_fw_size+=mHBV->EdgeObject(i).size();
        } else {
            join_split_fw++;
            join_split_fw_size+=mHBV->EdgeObject(i).size();
        }
    }
    std::cout<<"Forward Node Edge Classification: "<<std::endl
    <<"nothing_fw: "<<nothing_fw<<" ( "<<nothing_fw_size<<" kmers )"<<std::endl
    <<"line_fw: "<<line_fw<<" ( "<<line_fw_size<<" kmers )"<<std::endl
    <<"join_fw: "<<join_fw<<" ( "<<join_fw_size<<" kmers )"<<std::endl
    <<"split_fw: "<<split_fw<<" ( "<<split_fw_size<<" kmers )"<<std::endl
    <<"join_split_fw: "<<join_split_fw<<" ( "<<join_split_fw_size<<" kmers )"<<std::endl;

}

//void PathFinder_tx::unroll_loops(uint64_t min_side_sizes) {
//    //find nodes where in >1 or out>1 and in>0 and out>0
//    uint64_t uloop=0,ursize=0;
//    std::cout<<"Starting loop finding"<<std::endl;
//    init_prev_next_vectors();
//    std::cout<<"Prev and Next vectors initialised"<<std::endl;
//    //score all possible transitions, discards all decidible and
//
//        // is there any score>0 transition that is not incompatible with any other transitions?
//    std::vector<std::vector<uint64_t>> new_paths; //these are solved paths, they will be materialised later
//    for ( int e = 0; e < mHBV->EdgeObjectCount(); ++e ) {
//        if (e<mInv[e]) {
//            auto urs=is_unrollable_loop(e,min_side_sizes);
//
//            auto iurs=is_unrollable_loop(mInv[e],min_side_sizes);
//            if (urs.size()>0 && iurs.size()>0) {
//                //std::cout<<"unrolling loop on edge"<<e<<std::endl;
//                new_paths.push_back(urs[0]);
//            }
//        }
//
//    }
//    //std::cout<<"Unrollable loops: "<<uloop<<" ("<<ursize<<"bp)"<<std::endl;
//
//    std::cout<<"Loop finding finished, "<<new_paths.size()<< " loops to unroll" <<std::endl;
//    uint64_t sep=0;
//    std::map<uint64_t,std::vector<uint64_t>> old_edges_to_new;
//    for (auto p:new_paths){
//        auto oen=separate_path(p);
//        if (oen.size()>0) {
//            for (auto et:oen){
//                if (old_edges_to_new.count(et.first)==0) old_edges_to_new[et.first]={};
//                for (auto ne:et.second) old_edges_to_new[et.first].push_back(ne);
//            }
//            sep++;
//        }
//    }
//    if (old_edges_to_new.size()>0) {
//        migrate_readpaths(old_edges_to_new);
//    }
//    std::cout<<sep<<" loops unrolled, re-initing the prev and next vectors, just in case :D"<<std::endl;
//    init_prev_next_vectors();
//    std::cout<<"Prev and Next vectors initialised"<<std::endl;
//}
/*
void PathFinder_tx::untangle_complex_in_out_choices() {
    //find a complex path
    init_prev_next_vectors();
    for (int e = 0; e < mHBV->EdgeObjectCount(); ++e) {
        if (e < mInv[e] && mHBV->EdgeObject(e).size()>1000) {
            //Ok, so why do we stop?
            //is next edge a join? how long till it splits again? can we choose the split?
            if (next_edges[e].size()==1 and prev_edges[next_edges[e][0]].size()>1){
                //std::cout<<"next edge from "<<e<<" is a join!"<<std::endl;
                if (mHBV->EdgeObject(next_edges[e][0]).size()<500 and next_edges[next_edges[e][0]].size()>1){

                    if (prev_edges[next_edges[e][0]].size()==prev_edges[next_edges[e][0]].size()){
                        std::cout<<"next edge from "<<e<<" is a small join! with "<<prev_edges[next_edges[e][0]].size()<<"in-outs"<<std::endl;
                        auto join_edge=next_edges[e][0];
                        std::vector<std::vector<uint64_t>> p0011={
                                {prev_edges[join_edge][0],join_edge,next_edges[join_edge][0]},
                                {prev_edges[join_edge][1],join_edge,next_edges[join_edge][1]}
                        };
                        std::vector<std::vector<uint64_t>> p1001={
                                {prev_edges[join_edge][1],join_edge,next_edges[join_edge][0]},
                                {prev_edges[join_edge][0],join_edge,next_edges[join_edge][1]}
                        };
                        auto v0011=multi_path_votes(p0011);
                        auto v1001=multi_path_votes(p1001);
                        std::cout<<"votes for: "<<path_str(p0011[0])<<" / "<<path_str(p0011[1])<<":  "<<v0011[0]<<":"<<v0011[1]<<":"<<v0011[2]<<std::endl;
                        std::cout<<"votes for: "<<path_str(p1001[0])<<" / "<<path_str(p1001[1])<<":  "<<v1001[0]<<":"<<v1001[1]<<":"<<v1001[2]<<std::endl;
                    }
                }
            }

        }
    }
}*/

//void PathFinder_tx::untangle_pins() {
//
//    init_prev_next_vectors();
//    uint64_t pins=0;
//    for (int e = 0; e < mHBV->EdgeObjectCount(); ++e) {
//        if (mToLeft[e]==mToLeft[mInv[e]] and next_edges[e].size()==1 ) {
//            std::cout<<" Edge "<<e<<" forms a pinhole!!!"<<std::endl;
//            if (next_edges[next_edges[e][0]].size()==2) {
//                std::vector<uint64_t> pfw = {mInv[next_edges[next_edges[e][0]][0]],mInv[next_edges[e][0]],e,next_edges[e][0],next_edges[next_edges[e][0]][1]};
//                std::vector<uint64_t> pbw = {mInv[next_edges[next_edges[e][0]][1]],mInv[next_edges[e][0]],e,next_edges[e][0],next_edges[next_edges[e][0]][0]};
//                auto vpfw=multi_path_votes({pfw});
//                auto vpbw=multi_path_votes({pbw});
//                std::cout<<"votes FW: "<<vpfw[0]<<":"<<vpfw[1]<<":"<<vpfw[2]<<"     BW: "<<vpbw[0]<<":"<<vpbw[1]<<":"<<vpbw[2]<<std::endl;
//            }
//            ++pins;
//        }
//    }
//    std::cout<<"Total number of pinholes: "<<pins;
//}

LocalPaths::LocalPaths(HyperBasevector* hbv, std::vector<std::vector<uint64_t>> pair_solutions, vec<int> & to_right, TenXPather* txp, std::vector<BaseVec>& edges){
    mHBV = hbv;
    mTxp = txp;
    frontier_solutions = pair_solutions;
    mToRight = &to_right;
    mEdges = &edges;

    for (auto p: pair_solutions){
        ins.push_back(p[0]);
        outs.push_back(p[1]);
    }
}

bool LocalPaths::find_all_pair_conecting_paths(uint64_t edge_name , std::vector<uint64_t> path, int cont, uint64_t end_edge, int maxloop = 0) {
    // Find all paths between 2 given edges

    path.push_back(edge_name);
    cont ++;
    auto r_vertex = (*mToRight)[edge_name];

    // if the node is the end_edge or the run reaches a frontier limit the recursion completes !
    if (edge_name == end_edge){
        pair_temp_paths.push_back(path);

//        // DEBUGGING COUT!!
//        std::cout << "Found path: ";
//        for (auto x: path){
//            std::cout << x <<",";
//        }
//        std::cout << std::endl;//" / ";
//        // END DEBUGGING COUT

        return true;

    } else if (find(outs.begin(), outs.end(), edge_name) != outs.end()) {
        return false;
    }

    for (auto en=0; en<mHBV->FromSize(r_vertex); ++en){
        edge_name =mHBV->EdgeObjectIndexByIndexFrom(r_vertex, en);
        if (std::count(path.begin(), path.end(), edge_name) < maxloop+1){
            std::vector<uint64_t> subpath (path.begin(), path.begin()+cont);
            find_all_pair_conecting_paths(edge_name , subpath, cont, end_edge);
        }
    }
}

int LocalPaths::find_all_solution_paths(){
    // Get each one of the pairs for the frontier solutions finded and fill the edges and return every possible path
    // between the ends of each pair

    // For each of the pairs in the solved frontier pairs
    for (auto pn=0; pn < frontier_solutions.size(); ++pn){
        auto p_in = frontier_solutions[pn][0];
        auto p_out = frontier_solutions[pn][1];

        // Clear the temp pairs for this pair
        pair_temp_paths.clear();

        // Find all paths between the ends of the path
        std::vector<uint64_t> path;
        int cont = 0;
        auto status = find_all_pair_conecting_paths(p_in, path, cont, p_out);

        // Vote the best path from the all available
        auto best_path = choose_best_path(&pair_temp_paths);
        if (best_path.size()>0){
            all_paths.push_back(best_path);
        }
        else {
            std::cout << "Skipping an empty path between :" << p_in <<" and "<<  p_out << std::endl;
        }
    }
}

std::vector<uint64_t> LocalPaths::choose_best_path(std::vector<std::vector<uint64_t>>* alternative_paths){
    // Choose the best path for the combination from the pairs list

    // If there is only one posible path return that path and finish
    if (alternative_paths->size() == 0){
        std::cout <<"This path still CEROOOO" <<std::endl;
        std::vector<uint64_t> nopaths;
        return nopaths;
    }
    if (alternative_paths->size() == 1){
        std::cout << "Only one patha available: " << std::endl;
        return (*alternative_paths)[0];
    } else {
        // If there is more than one path vote for the best (the criteria here is most tag density (presentTags/totalKmers)
        float best_path = 0.0;
        float best_path_score = 0.0;
        for (auto path_index = 0; path_index < alternative_paths->size(); ++path_index) {
            float cpath_score = 0;
            for (auto ei = 0; ei < (*alternative_paths)[path_index].size() - 1; ++ei) {
                auto from_edge_string = (*mEdges)[(*alternative_paths)[path_index][ei]].ToString();
                auto to_edge_string = (*mEdges)[(*alternative_paths)[path_index][ei + 1]].ToString();
                cpath_score += mTxp->edgeTagIntersection(from_edge_string, to_edge_string, 1500);
            }
            if (cpath_score > best_path_score) {
                best_path = path_index;
                best_path_score = cpath_score;
            }
        }
        std::cout << "Best path selected: " << best_path << ", score: " << best_path_score << std::endl;
        for (auto p=0; p<(*alternative_paths)[best_path].size(); ++p){
            std::cout << (*alternative_paths)[best_path][p] << ",";
        }
        std::cout << std::endl;

        return (*alternative_paths)[best_path];
    }
}


void PathFinder_tx::untangle_complex_in_out_choices(uint64_t large_frontier_size, bool verbose_separation) {
    //find a complex path
    auto edges = mHBV->Edges();

    uint64_t qsf=0,qsf_paths=0;
    uint64_t msf=0,msf_paths=0;
    init_prev_next_vectors();
    std::cout<<"vectors initialised"<<std::endl;
    std::set<std::array<std::vector<uint64_t>,2>> seen_frontiers,solved_frontiers;
    std::vector<std::vector<uint64_t>> paths_to_separate;
    int solved_regions = 0;
    int unsolved_regions = 0;
    for (int e = 0; e < mHBV->EdgeObjectCount(); ++e) {
        if (e < mInv[e] && mHBV->EdgeObject(e).size() < large_frontier_size) {
            // Get the frontiers fo the edge
            // [GONZA] TODO: check the return details to document
            auto f=get_all_long_frontiers(e, large_frontier_size);
            if (f[0].size()>1 and f[1].size()>1 and f[0].size() == f[1].size() and seen_frontiers.count(f)==0){
                seen_frontiers.insert(f);
                bool single_dir=true;
                std::map<std::string, float> shared_paths;
                for (auto in_e:f[0]) for (auto out_e:f[1]) if (in_e==out_e) {single_dir=false;break;}

                if (single_dir) {
                    // If there is a region to resolve within the frontiers
                    std::cout<<" Single direction frontiers for complex region on edge "<<e<<" IN:"<<path_str(f[0])<<" OUT: "<<path_str(f[1])<<std::endl;
                    std::vector<int> in_used(f[0].size(),0);
                    std::vector<int> out_used(f[1].size(),0);
                    std::vector<std::vector<uint64_t>> first_full_paths;
                    bool reversed=false;

                    // intersect all pairs of ins and outs to score, save the scores in a map to score the combinations later
                    for (auto in_i=0;in_i<f[0].size();++in_i) {
                        auto in_e=f[0][in_i];
                        for (auto out_i=0;out_i<f[1].size();++out_i) {
                            auto out_e=f[1][out_i];

                            int edges_in_path;
                            std::string pid;
                            auto in_e_seq = edges[in_e].ToString();
                            auto out_e_seq = edges[out_e].ToString();

                            // Intersect the tags for the edges
                            auto intersection_score = mTxp->edgeTagIntersection(in_e_seq, out_e_seq, 1500);
//                            std::cout << "Intersection score: " << intersection_score << std::endl;
                            if (intersection_score>0.0001){
                                // if the edges overlap in the tagspace thay are added to the map and the combination is markes in the used edges
                                pid = std::to_string(in_e) + "-" + std::to_string(out_e);
                                shared_paths[pid] += intersection_score; // This should score the link based in the number of tags that tha pair shares
//                                std::cout << "Accumulated for "<<pid<<":"<< shared_paths[pid] << std::endl;
                                out_used[out_i]++;
                                in_used[in_i]++;
                            }
                        }
                    } // When this is done i get a map of all combinations of ins and outs to score the permutations in the next step

                    // Here all combinations are counted, now i need to get the best configuration between nodes
                    auto in_frontiers = f[0];
                    auto out_frontiers = f[1];
                    float max_score = -9999.0;
                    int perm_number = 0;

                    std::vector<int> max_score_permutation;
                    do {
                        // Vectors to count seen edges (check that all edges are included in th solution)
                        std::vector<int> seen_in(in_frontiers.size(), 0);
                        std::vector<int> seen_out(in_frontiers.size(), 0);

                        //std::cout << "-----------------------------Testing permutaiton ------------------" << std::endl;
                        float current_score = 0;
                        for (auto pi=0; pi<in_frontiers.size(); ++pi){
                            std::string index = std::to_string(in_frontiers[pi])+"-"+std::to_string(out_frontiers[pi]);
                            if ( shared_paths.find(index) != shared_paths.end() ){
                                // Mark the pair as seen in this iteration
                                seen_in[pi]++;
                                seen_out[pi]++;
                                current_score += shared_paths[index];
                            }
                        }
                        // Check that all boundaries are used in the permutation
                        bool all_used = true;
                        for (auto a=0; a<seen_in.size(); ++a){
                            if (0==seen_in[a] or 0==seen_out[a]){
                                all_used = false;
                            }
                        }

                        if (current_score>max_score and all_used){
                            max_score = current_score;
                            max_score_permutation.clear();
                            for (auto aix: out_frontiers){
                                max_score_permutation.push_back(aix);
                            }
                        }
                        perm_number++;
                    } while (std::next_permutation(out_frontiers.begin(), out_frontiers.end()));

                    // Get the solution
                    float score_threshold = 0.0001;
                    if (max_score>score_threshold){
                        std::cout << " Found solution to region: " <<std::endl;
                        solved_regions++;

                        std::vector<std::vector<uint64_t>> wining_permutation;
                        for (auto ri=0; ri<max_score_permutation.size(); ++ri){
                            std::cout << in_frontiers[ri] << "(" << mInv[in_frontiers[ri]] << ") --> " << max_score_permutation[ri] << "("<< mInv[max_score_permutation[ri]] <<"), Score: "<<  max_score << std::endl;
                            std::vector<uint64_t> tp = {in_frontiers[ri], max_score_permutation[ri]};
                            wining_permutation.push_back(tp);
                        }
                        std::cout << "--------------------" << std::endl;

                        // Fill intermediate nodes
                        LocalPaths lp (mHBV, wining_permutation, mToRight, mTxp, edges);
                        lp.find_all_solution_paths();
                        std::cout << "All paths done" << std::endl;
                        for (auto spi = 0; spi<lp.all_paths.size(); ++spi){
                            auto sv = lp.all_paths[spi];
                            paths_to_separate.push_back(sv);
                        }
                        std::cout << "--------------------" << std::endl;
                    } else {
//                        std::cout << "Region not resolved, not enough links or bad combinations" << max_score << std::endl;
                        unsolved_regions++;
                    }

                }
            }
        }
    }
    std::cout << "========================" << std::endl;
    std::cout << "Solved regions: " << solved_regions << ", Unsolved regions: " << unsolved_regions << " --> " << (float)solved_regions / (float)(solved_regions + unsolved_regions) * 100 << " %" << std::endl;
    std::cout << "========================" << std::endl;

//    std::cout<<"Complex Regions solved by paths: "<<solved_frontiers.size() <<"/"<<seen_frontiers.size()<<" comprising "<<paths_to_separate.size()<<" paths to separate"<< std::endl;
//    //std::cout<<"Complex Regions quasi-solved by paths (not acted on): "<< qsf <<"/"<<seen_frontiers.size()<<" comprising "<<qsf_paths<<" paths to separate"<< std::endl;
//    //std::cout<<"Multiple Solution Regions (not acted on): "<< msf <<"/"<<seen_frontiers.size()<<" comprising "<<msf_paths<<" paths to separate"<< std::endl;
    std::cout << "Paths to separate" << std::endl;

    for (auto ss: paths_to_separate){
      for (auto sx: ss){
        std::cout << sx << ",";
      }
      std::cout << std::endl;
    }
    std::cout << "Paths to separate: " << paths_to_separate.size() << std::endl;

    uint64_t sep=0;
    std::map<uint64_t,std::vector<uint64_t>> old_edges_to_new;
    for (auto p: paths_to_separate){
        if (old_edges_to_new.count(p.front()) > 0 or old_edges_to_new.count(p.back()) > 0) {
            std::cout<<"WARNING: path starts or ends in an already modified edge, skipping"<<std::endl;
            continue;
        }

        auto oen=separate_path(p, verbose_separation);
        std::cout << "End separate paths, eon size: " << oen.size() << std::endl;
        if (oen.size()>0) {
            for (auto et:oen){
                if (old_edges_to_new.count(et.first)==0) old_edges_to_new[et.first]={};
                for (auto ne:et.second) old_edges_to_new[et.first].push_back(ne);
            }
            sep++;
            std::cout << "separated_paths: " << sep << std::endl;
        }
    }
//    if (old_edges_to_new.size()>0) {
//        migrate_readpaths(old_edges_to_new);
//    }
    std::cout<<" "<<sep<<" paths separated!"<<std::endl;
}

std::vector<std::vector<uint64_t>> PathFinder_tx::AllPathsFromTo(std::vector<uint64_t> in_edges, std::vector<uint64_t> out_edges, uint64_t max_length) {
    std::vector<std::vector<uint64_t>> current_paths;
    std::vector<std::vector<uint64_t>> paths;
    //First, start all paths from in_edges
    for (auto ine:in_edges) current_paths.push_back({ine});
    while (max_length--) {
        auto old_paths=current_paths;
        current_paths.clear();
        for (auto op:old_paths) {
            //grow each path, adding variations if needed
            for (auto ne:next_edges[op.back()]){
                //if new edge on out_edges, add to paths
                op.push_back(ne);
                if (std::count(out_edges.begin(),out_edges.end(),ne)) paths.push_back(op);
                else current_paths.push_back(op);
            }
        }
    }
    return paths;

}

std::array<std::vector<uint64_t>,2> PathFinder_tx::get_all_long_frontiers(uint64_t e, uint64_t large_frontier_size){
    //TODO: return all components in the region
    std::set<uint64_t> seen_edges, to_explore={e}, in_frontiers, out_frontiers;

    while (to_explore.size()>0){
        std::set<uint64_t> next_to_explore;
        for (auto x:to_explore){ //to_explore: replace rather and "update" (use next_to_explore)

            if (!seen_edges.count(x)){

                //What about reverse complements and paths that include loops that "reverse the flow"?
                if (seen_edges.count(mInv[x])) return std::array<std::vector<uint64_t>,2>(); //just cancel for now

                for (auto p:prev_edges[x]) {
                    if (mHBV->EdgeObject(p).size() >= large_frontier_size )  {
                        //What about frontiers on both sides?
                        in_frontiers.insert(p);
                        for (auto other_n:next_edges[p]){
                            if (!seen_edges.count(other_n)) {
                                if (mHBV->EdgeObject(other_n).size() >= large_frontier_size) {
                                    out_frontiers.insert(other_n);
                                    seen_edges.insert(other_n);
                                }
                                else next_to_explore.insert(other_n);
                            }
                        }
                    }
                    else if (!seen_edges.count(p)) next_to_explore.insert(p);
                }

                for (auto n:next_edges[x]) {
                    if (mHBV->EdgeObject(n).size() >= large_frontier_size) {
                        //What about frontiers on both sides?
                        out_frontiers.insert(n);
                        for (auto other_p:prev_edges[n]){
                            if (!seen_edges.count(other_p)) {
                                if (mHBV->EdgeObject(other_p).size() >= large_frontier_size) {
                                    in_frontiers.insert(other_p);
                                    seen_edges.insert(other_p);
                                }
                                else next_to_explore.insert(other_p);
                            }
                        }
                    }
                    else if (!seen_edges.count(n)) next_to_explore.insert(n);
                }
                seen_edges.insert(x);
            }
            if (seen_edges.size()>50) {
                return std::array<std::vector<uint64_t>,2>();
            }

        }
        to_explore=next_to_explore;
    }

    if (to_explore.size()>0) return std::array<std::vector<uint64_t>,2>();
    //the "canonical" representation is the one that has the smalled edge on the first vector, and bot ordered

    if (in_frontiers.size()>0 and out_frontiers.size()>0) {
        uint64_t min_in=*in_frontiers.begin();
        for (auto i:in_frontiers){
            if (i<min_in) min_in=i;
            if (mInv[i]<min_in) min_in=mInv[i];
        }
        uint64_t min_out=*out_frontiers.begin();
        for (auto i:out_frontiers){
            if (i<min_out) min_out=i;
            if (mInv[i]<min_out) min_out=mInv[i];
        }
        if (min_out<min_in){
            std::set<uint64_t> new_in, new_out;
            for (auto e:in_frontiers) new_out.insert(mInv[e]);
            for (auto e:out_frontiers) new_in.insert(mInv[e]);
            in_frontiers=new_in;
            out_frontiers=new_out;
        }
    }

    //std::sort(in_frontiers.begin(),in_frontiers.end());
    //std::sort(out_frontiers.begin(),out_frontiers.end());
    std::array<std::vector<uint64_t>,2> frontiers={std::vector<uint64_t>(in_frontiers.begin(),in_frontiers.end()),std::vector<uint64_t>(out_frontiers.begin(),out_frontiers.end())};



    return frontiers;
}

std::vector<std::vector<uint64_t>> PathFinder_tx::is_unrollable_loop(uint64_t loop_e, uint64_t min_size_sizes){
    //Checks if loop can be unrolled
    //Input: looping edge (i.e. prev_e--->R---->loop_e---->R---->next_e)
    uint64_t prev_e, repeat_e, next_e;
    //Conditions for unrollable loop:

    //1) only one neighbour on each direction, and the same one (repeat_e).
    if (prev_edges[loop_e].size()!=1 or
        next_edges[loop_e].size()!=1 or
        prev_edges[loop_e][0]!=next_edges[loop_e][0]) return {};
    repeat_e=prev_edges[loop_e][0];


    //2) the repeat edge has only one other neighbour on each direction, and it is a different one;
    if (prev_edges[repeat_e].size()!=2 or
        next_edges[repeat_e].size()!=2) return {};

    prev_e=(prev_edges[repeat_e][0]==loop_e ? prev_edges[repeat_e][1]:prev_edges[repeat_e][0]);

    next_e=(next_edges[repeat_e][0]==loop_e ? next_edges[repeat_e][1]:next_edges[repeat_e][0]);

    if (prev_e==next_e or prev_e==mInv[next_e]) return {};

    //3) size constraints: prev_e and next_e must be at least 1Kbp
    if (mHBV->EdgeObject(prev_e).size()<min_size_sizes or mHBV->EdgeObject(next_e).size()<min_size_sizes) return {};


    //std::cout<<" LOOP: "<< edge_pstr(prev_e)<<" ---> "<<edge_pstr(repeat_e)<<" -> "<<edge_pstr(loop_e)<<" -> "<<edge_pstr(repeat_e)<<" ---> "<<edge_pstr(next_e)<<std::endl;
    auto pvlin=path_votes({prev_e,repeat_e,loop_e,repeat_e,next_e});
    auto pvloop=path_votes({prev_e,repeat_e,loop_e,repeat_e,loop_e,repeat_e,next_e});
    //std::cout<<" Votes to p->r->l->r->n: "<<pvlin[0]<<":"<<pvlin[1]<<":"<<pvlin[2]<<std::endl;
    //std::cout<<" Votes to p->r->l->r->l->r->n: "<<pvloop[0]<<":"<<pvloop[1]<<":"<<pvloop[2]<<std::endl;

    auto pvcircleline=multi_path_votes({{loop_e,repeat_e,loop_e},{prev_e,repeat_e,next_e}});
    //std::cout<<" Multi-votes to circle+line "<<pvcircleline[0]<<":"<<pvcircleline[1]<<":"<<pvcircleline[2]<<std::endl;

    if (pvcircleline[0]>0 or pvloop[2]>0 or (pvcircleline[2]==0 and pvcircleline[1]>pvlin[1] and pvcircleline[1]>pvloop[1])) {
        //std::cout<<"   CAN'T be reliably unrolled!"<<std::endl;
        return {};
    }
    if (pvloop[3]==0 and pvloop[0]>pvlin[0]){
        //std::cout<<"   LOOP should be traversed as loop at least once!"<<std::endl;
        return {};
    }
    if (pvlin==pvcircleline){
        //std::cout<<"   UNDECIDABLE as path support problem, looking at coverages"<<std::endl;
        float prev_cov=paths_per_kbp(prev_e);
        float repeat_cov=paths_per_kbp(repeat_e);
        float loop_cov=paths_per_kbp(loop_e);
        float next_cov=paths_per_kbp(next_e);
        auto sc_min=prev_cov*.8;
        auto sc_max=prev_cov*1.2;
        auto dc_min=prev_cov*1.8;
        auto dc_max=prev_cov*2.2;
        if (repeat_cov<dc_min or repeat_cov>dc_max or
                loop_cov<sc_min or loop_cov>sc_max or
                next_cov<sc_min or next_cov>sc_max ){
            //std::cout<<"    Coverage FAIL conditions, CAN'T be reliably unrolled!"<<std::endl;
            return {};
        }
        //std::cout<<"    Coverage OK"<<std::endl;
    }
    //std::cout<<"   UNROLLABLE"<<std::endl;
    return {{prev_e,repeat_e,loop_e,repeat_e,next_e}};
    //return mHBV->EdgeObject(prev_e).size()+mHBV->EdgeObject(repeat_e).size()+
            //mHBV->EdgeObject(loop_e).size()+mHBV->EdgeObject(repeat_e).size()+mHBV->EdgeObject(next_e).size()-4*mHBV->K();
}

uint64_t PathFinder_tx::paths_per_kbp(uint64_t e){
    return 1000 * mEdgeToPathIds[e].size()/mHBV->EdgeObject(e).size();
};

std::map<uint64_t,std::vector<uint64_t>> PathFinder_tx::separate_path(std::vector<uint64_t> p, bool verbose_separation){

    //TODO XXX: proposed version 1 (never implemented)
    //Creates new edges for the "repeaty" parts of the path (either those shared with other edges or those appearing multiple times in this path).
    //moves paths across to the new reapeat instances as needed
    //changes neighbourhood (i.e. creates new vertices and moves the to and from for the implicated edges).

    //creates a copy of each node but the first and the last, connects only linearly to the previous copy,
    //std::cout<<std::endl<<"Separating path"<<std::endl;
    std::set<uint64_t> edges_fw;
    std::set<uint64_t> edges_rev;
    for (auto e:p){//TODO: this is far too astringent...
        edges_fw.insert(e);
        edges_rev.insert(mInv[e]);

        if (edges_fw.count(mInv[e]) ||edges_rev.count(e) ){ //std::cout<<"PALINDROME edge detected, aborting!!!!"<<std::endl;
            return {};}
    }
    //create two new vertices (for the FW and BW path)
    uint64_t current_vertex_fw=mHBV->N(),current_vertex_rev=mHBV->N()+1;
    mHBV->AddVertices(2);
    //migrate connections (dangerous!!!)
    if (verbose_separation) std::cout<<"Migrating edge "<<p[0]<<" To node old: "<<mToRight[p[0]]<<" new: "<<current_vertex_fw<<std::endl;
    mHBV->GiveEdgeNewToVx(p[0],mToRight[p[0]],current_vertex_fw);
    if (verbose_separation) std::cout<<"Migrating edge "<<mInv[p[0]]<<" From node old: "<<mToLeft[mInv[p[0]]]<<" new: "<<current_vertex_rev<<std::endl;
    mHBV->GiveEdgeNewFromVx(mInv[p[0]],mToLeft[mInv[p[0]]],current_vertex_rev);
    std::map<uint64_t,std::vector<uint64_t>> old_edges_to_new;

    for (auto ei=1;ei<p.size()-1;++ei){
        //add a new vertex for each of FW and BW paths
        uint64_t prev_vertex_fw=current_vertex_fw,prev_vertex_rev=current_vertex_rev;
        //create two new vertices (for the FW and BW path)
        current_vertex_fw=mHBV->N();
        current_vertex_rev=mHBV->N()+1;
        mHBV->AddVertices(2);

        //now, duplicate next edge for the FW and reverse path
        auto nef=mHBV->AddEdge(prev_vertex_fw,current_vertex_fw,mHBV->EdgeObject(p[ei]));
        if (verbose_separation)  std::cout<<"Edge "<<nef<<": copy of "<<p[ei]<<": "<<prev_vertex_fw<<" - "<<current_vertex_fw<<std::endl;
        mToLeft.push_back(prev_vertex_fw);
        mToRight.push_back(current_vertex_fw);
        if (! old_edges_to_new.count(p[ei]))  old_edges_to_new[p[ei]]={};
        old_edges_to_new[p[ei]].push_back(nef);

        auto ner=mHBV->AddEdge(current_vertex_rev,prev_vertex_rev,mHBV->EdgeObject(mInv[p[ei]]));
        if (verbose_separation) std::cout<<"Edge "<<ner<<": copy of "<<mInv[p[ei]]<<": "<<current_vertex_rev<<" - "<<prev_vertex_rev<<std::endl;
        mToLeft.push_back(current_vertex_rev);
        mToRight.push_back(prev_vertex_rev);
        if (! old_edges_to_new.count(mInv[p[ei]]))  old_edges_to_new[mInv[p[ei]]]={};
        old_edges_to_new[mInv[p[ei]]].push_back(ner);
        std::cout << "before pushing" << std::endl;
        mInv.push_back(ner);
        mInv.push_back(nef);
//        mEdgeToPathIds.resize(mEdgeToPathIds.size()+2); // [GONZA] TODO: this fals for some reason commented now

    }
    if (verbose_separation) std::cout<<"Migrating edge "<<p[p.size()-1]<<" From node old: "<<mToLeft[p[p.size()-1]]<<" new: "<<current_vertex_fw<<std::endl;
    mHBV->GiveEdgeNewFromVx(p[p.size()-1],mToLeft[p[p.size()-1]],current_vertex_fw);
    if (verbose_separation) std::cout<<"Migrating edge "<<mInv[p[p.size()-1]]<<" To node old: "<<mToRight[mInv[p[p.size()-1]]]<<" new: "<<current_vertex_rev<<std::endl;
    mHBV->GiveEdgeNewToVx(mInv[p[p.size()-1]],mToRight[mInv[p[p.size()-1]]],current_vertex_rev);

    //TODO: cleanup new isolated elements and leading-nowhere paths.
    //for (auto ei=1;ei<p.size()-1;++ei) mHBV->DeleteEdges({p[ei]});
    return old_edges_to_new;

}

//TODO: this should probably be called just once
void PathFinder_tx::migrate_readpaths(std::map<uint64_t,std::vector<uint64_t>> edgemap){
    //Migrate readpaths: this changes the readpaths from old edges to new edges
    //if an old edge has more than one new edge it tries all combinations until it gets the paths to map
    //if more than one combination is valid, this chooses at random among them (could be done better? should the path be duplicated?)
    mHBV->ToLeft(mToLeft);
    mHBV->ToRight(mToRight);
    for (auto &p:mPaths){
        std::vector<std::vector<uint64_t>> possible_new_edges;
        bool translated=false,ambiguous=false;
        for (auto i=0;i<p.size();++i){
            if (edgemap.count(p[i])) {
                possible_new_edges.push_back(edgemap[p[i]]);
                if (not translated) translated=true;
                if (possible_new_edges.back().size()>1) ambiguous=true;
            }
            else possible_new_edges.push_back({p[i]});
        }
        if (translated){
            if (not ambiguous){ //just straigh forward translation
                for (auto i=0;i<p.size();++i) p[i]=possible_new_edges[i][0];
            }
            else {
                //ok, this is the complicated case, we first generate all possible combinations
                std::vector<std::vector<uint64_t>> possible_paths={{}};
                for (auto i=0;i<p.size();++i) {//for each position
                    std::vector<std::vector<uint64_t>> new_possible_paths;
                    for (auto pp:possible_paths) { //take every possible one
                        for (auto e:possible_new_edges[i]) {
                            //if i>0 check there is a real connection to the previous edge
                            if (i == 0 or (mToRight[pp.back()]==mToLeft[e])) {
                                new_possible_paths.push_back(pp);
                                new_possible_paths.back().push_back(e);
                            }
                        }
                    }
                    possible_paths=new_possible_paths;
                    if (possible_paths.size()==0) break;
                }
                if (possible_paths.size()==0){
                    std::cout<<"Warning, a path could not be updated, truncating it to its first element!!!!"<<std::endl;
                    p.resize(1);
                }
                else{
                    std::srand (std::time(NULL));
                    //randomly choose a path
                    int r=std::rand()%possible_paths.size();
                    for (auto i=0;i<p.size();++i) p[i]=possible_paths[r][i];
                }
            }
        }
    }

}

bool PathFinder_tx::join_edges_in_path(std::vector<uint64_t> p){
    //if a path is scrictly linear, it joins it
}