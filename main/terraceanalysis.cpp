//
//  terraceanalysis.cpp
//  iqtree
//
//  Created by Olga on 10.09.20.
//

#include "terraceanalysis.hpp"
#include "terrace/terracenode.hpp"
#include "terrace/terracetree.hpp"
#include "terrace/terrace.hpp"
#include "terrace/presenceabsencematrix.hpp"
#include "tree/mtreeset.h"
#include "alignment/superalignment.h"
#include "utils/timeutil.h"


void runterraceanalysis(Params &params){
    
    params.startCPUTime = getCPUTime();
    params.start_real_time = getRealTime();
    
    
    if(params.matrix_order){
        cout<<"Reordering presence-absence matrix..\n\n";
        PresenceAbsenceMatrix *matrix = new PresenceAbsenceMatrix();
        if(params.partition_file && params.aln_file){
            matrix->get_from_alignment(params);
        }else{
            assert(params.pr_ab_matrix && "ERROR: no input presence-absence matrix!");
            matrix->read_pr_ab_matrix(params.pr_ab_matrix);
        }
        matrix->print_pr_ab_matrix();
        matrix->getPartOverlapComplete();
        matrix->print_overlap_matrix();
        
        cout<<"Total wall-clock time used: "<<getRealTime()-Params::getInstance().start_real_time<<" seconds ("<<convert_time(getRealTime()-Params::getInstance().start_real_time)<<")\n";
        cout<<"Total CPU time used: "<< getCPUTime()-Params::getInstance().startCPUTime << " seconds (" << convert_time(getCPUTime()-Params::getInstance().startCPUTime) << ")\n\n";
        
        time_t end_time;
        time(&end_time);
        cout << "Date and Time: " << ctime(&end_time);
        exit(0);
    }
    
    
    /*
     *  This is an actual terrace:
     *  - main tree - is terrace representative tree
     *  - induced partition trees define the terrace
     */
    Terrace *terrace;
    
    if(params.partition_file && params.aln_file && params.user_file){
        PresenceAbsenceMatrix *matrix = new PresenceAbsenceMatrix();
        matrix->get_from_alignment(params);
        TerraceTree tree;
        tree.readTree(params.user_file, params.is_rooted);
        terrace = new Terrace(tree,matrix);
    } else if(params.user_file && params.pr_ab_matrix){
        terrace = new Terrace(params.user_file,params.is_rooted,params.pr_ab_matrix);
    } else {
        throw "ERROR: to start terrace analysis input a tree and either a presence-absence matrix or alignment with partition info!";
    }
    
    /*terrace->out_file = params.out_prefix;
    terrace->out_file += ".terrace_info";
    
    ofstream out_terrace_info;
    out_terrace_info.exceptions(ios::failbit | ios::badbit);
    out_terrace_info.open(terrace->out_file);
    
    out_terrace_info.close();*/
    
    
    if(params.terrace_query_set){
        run_terrace_check(terrace,params);
    }else if(params.print_induced_trees){
        terrace->printInfo();
    }else{
        /*
         *  Create an auxiliary terrace:
         *  - main tree - is the initial tree to be expanded by adding taxa to obtain a tree from the considered terrace (or to reach dead end)
         *  - induced trees - the common subtrees of the main tree and higher level induced partition tree, respectivelly per partition
         *
         *  There will be also a vector of terraces with just one partition. Per partition each terrace is a pair of high and low (common) induced partition trees
         *  - main tree - high level partition tree
         *  - induced tree - a common subtree between "initial to be expanded main tree" and high level induced tree
         */
    
        //terrace->printInfo();
        terrace->matrix->print_pr_ab_matrix();
    
        // INITIAL TREE (idealy should be the largest subtree without unique species, a subtree of some induced partition tree)
        vector<string> taxa_names_sub;
        // Get a list of taxa to be inserted.
        vector<string> list_taxa_to_insert;
        terrace->matrix->getINFO_init_tree_taxon_order(taxa_names_sub,list_taxa_to_insert);
        
        // SANITY:
        //list_taxa_to_insert.clear();
        
        // INITIAL TREE is the largest partition tree. If the above function is used, this one should not be used
        /*int init_part = 0;
        for(i=0; i<terrace->taxa_num; i++){
            if(terrace->matrix->pr_ab_matrix[i][init_part]==1){
                taxa_names_sub.push_back(terrace->matrix->taxa_names[i]);
                //cout<<"TAXON "<<terrace->matrix->taxa_names[i]<<endl;
            }
        }*/
    
        // INITIAL TERRACE (to be used for generating trees, representative is the initial tree and induced partition trees are common subtrees with induced partition trees of considered terrace from above)
        // Creating a subterrace: submatrix and an initial tree
        PresenceAbsenceMatrix *submatrix = new PresenceAbsenceMatrix();
        terrace->matrix->getSubPrAbMatrix(taxa_names_sub, submatrix);
    
        TerraceTree tree_init;
        tree_init.copyTree_byTaxonNames(terrace,taxa_names_sub);
        //tree_init.drawTree(cout, WT_BR_SCALE | WT_TAXON_ID | WT_NEWLINE);
        //cout<<"INITIAL TERRACE"<<endl;
        Terrace *init_terrace = new Terrace(tree_init, submatrix);

        init_terrace->out_file = params.out_prefix;
        init_terrace->out_file += ".all_gen_terrace_trees";
        
        if(params.terrace_stop_intermediate_num > 0){
            init_terrace->intermediate_max_trees = params.terrace_stop_intermediate_num;
        }
        if(params.terrace_stop_terrace_trees_num > 0){
            init_terrace->terrace_max_trees = params.terrace_stop_terrace_trees_num;
        }
        if(params.terrace_stop_time > 0){
            init_terrace->seconds_max = params.terrace_stop_time*3600;
        }
        init_terrace->trees_out_lim = params.terrace_print_lim;
        //cout<<"PRINTING LIMIT:"<<init_terrace->trees_out_lim<<endl;
        
        init_terrace->linkTrees(true, false); // branch_back_map, taxon_back_map; in this case you only want to map branches

        vector<Terrace*> part_tree_pairs;
        init_terrace->create_Top_Low_Part_Tree_Pairs(part_tree_pairs, terrace);
        
        // ORDER: no specific order, i.e. based on the input presence-absence matrix the next taxon not present on initial tree.
        // if this function is used (terrace->matrix->getINFO_init_tree_taxon_order(taxa_names_sub,list_taxa_to_insert);), the below code shouldn't be used
        /*for(i=0; i<terrace->taxa_num; i++){
            if(init_terrace->matrix->findTaxonID(terrace->matrix->taxa_names[i])==-1){
               list_taxa_to_insert.push_back(terrace->matrix->taxa_names[i]);
            }
        }*/
        
        /*cout<<"Taxa to insert on initial tree: "<<endl;
        for(i=0; i<list_taxa_to_insert.size(); i++){
            cout<<i<<":"<<list_taxa_to_insert[i]<<endl;
        }*/
    
        // TAXON ORDER 3: Based on the number of allowed branches.
        vector<string> ordered_taxa_to_insert;
        ordered_taxa_to_insert = list_taxa_to_insert;
        
        cout<<"\n"<<"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++"<<"\n";
        cout<<"\n"<<"READY TO GENERATE TREES FROM A TERRACE"<<"\n";
        cout<<"\n"<<"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++"<<"\n";
        cout<<"INPUT INFO:"<<"\n";
        cout<<"---------------------------------------------------------"<<"\n";
        cout<<"Number of taxa: "<<terrace->taxa_num<<"\n";
        cout<<"Number of partitions: "<<terrace->part_num<<"\n";
        terrace->matrix->percent_missing();
        cout<<"% of missing entries in supermatrix: "<<terrace->matrix->missing_percent<<"\n";
        cout<<"Number of taxa on initial tree: "<<init_terrace->taxa_num<<"\n";
        cout<<"Number of taxa to be inserted: "<<list_taxa_to_insert.size()<<"\n";
    
    
        //init_terrace->print_ALL_DATA(part_tree_pairs);
        if(params.print_terrace_trees){
            ofstream out;
            out.exceptions(ios::failbit | ios::badbit);
            out.open(init_terrace->out_file);
            out.close();
            
            init_terrace->out.exceptions(ios::failbit | ios::badbit);
            init_terrace->out.open(init_terrace->out_file,std::ios_base::app);
            
        }else{
            init_terrace->terrace_out = false;
        }
        
        cout<<endl<<"Generating terrace trees.."<<endl;
        init_terrace->generateTerraceTrees(terrace, part_tree_pairs, list_taxa_to_insert, 0, &ordered_taxa_to_insert);
        cout<<endl<<"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++"<<endl;
        cout<<endl<<"Done!"<<endl<<endl;

        init_terrace->write_summary_generation();
    }

};

void run_terrace_check(Terrace *terrace,Params &params){
    
    int i, count = 0;
    vector<MTree*> trees_on, trees_off;
    
    /*MTreeSet tree_set_query(params.terrace_query_set, params.is_rooted, params.tree_burnin, params.tree_max_count);
    for(i=0; i<tree_set_query.size(); i++){
        if(terrace->check_two_trees(tree_set_query[i])){
            trees_on.push_back(tree_set_query[i]);
        }else{
            trees_off.push_back(tree_set_query[i]);
        }
    }*/
    
    // Alternative
    ifstream *in = new ifstream;
    in->open(params.terrace_query_set);
    while (!in->eof() && count < params.tree_max_count) {
        count++;
        MTree *tree = new MTree();
        tree->readTree(*in, params.is_rooted);
        if(terrace->check_two_trees(tree)){
            cout<<"Checking query tree "<<count<<"..."<<"on the terrace."<<endl;
            trees_on.push_back(tree);
        }else{
            cout<<"Checking query tree "<<count<<"..."<<"NOT on the terrace!"<<endl;
            trees_off.push_back(tree);
        }
        delete tree;
        
    }
    in->close();
    delete in;
    
    if(!trees_on.empty() and !trees_off.empty()){
        string out_file_on = params.out_prefix;
        out_file_on += ".on_terrace";
        
        ofstream out_on;
        out_on.exceptions(ios::failbit | ios::badbit);
        out_on.open(out_file_on);
        
        for(i=0; i<trees_on.size(); i++){
            trees_on[i]->printTree(out_on,WT_BR_SCALE | WT_NEWLINE);
        }
        
        out_on.close();
    }

    if(!trees_off.empty()){
        string out_file_off = params.out_prefix;
        out_file_off += ".off_terrace";
    
        ofstream out_off;
        out_off.exceptions(ios::failbit | ios::badbit);
        out_off.open(out_file_off);
        
        for(i=0; i<trees_off.size(); i++){
            trees_off[i]->printTree(out_off,WT_BR_SCALE | WT_NEWLINE);
        }
        
        out_off.close();
    }
    
    cout<<endl<<"----------------------------------------------------------------------------"<<endl;
    cout<<"Done."<<endl;
    cout<<"Checked "<<count<<" trees:"<<endl;
    if(trees_on.size() == count){
        cout<<"- all trees belong to the same terrace"<<endl;
    }else if(trees_off.size() == count){
        cout<<"- none of the trees belongs to considered terrace"<<endl;
    }else{
        cout<<" - "<<trees_on.size()<<" trees belong to considered terrace"<<endl;
        cout<<" - "<<trees_off.size()<<" trees do not belong to considered terrace"<<endl;
    }
    cout<<"----------------------------------------------------------------------------"<<endl<<endl;
}

