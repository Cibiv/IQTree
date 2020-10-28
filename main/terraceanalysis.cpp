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

void runterraceanalysis(Params &params){
    
    int i=0, j=0;
    
    if(params.user_file && params.pr_ab_matrix){
        
        /*  INFO:
         *  This is an actual terrace:
         *  - main tree - is terrace representative tree
         *  - induced partition trees define the terrace
         *
         *  Create an auxiliary terrace:
         *  - main tree - is the initial tree to be expanded by adding taxa to obtain a tree from the considered terrace (or to reach dead end)
         *  - induced trees - the common subtrees of the main tree and higher level induced partition tree, respectivelly per partition
         *
         *  There will be also a vector of terraces with just one partition. Per partition each terrace is a pair of high and low (common) induced partition trees
         *  - main tree - high level partition tree
         *  - induced tree - a common subtree between "initial to be expanded main tree" and high level induced tree
         */
        
        // CONSIDERED TERRACE (to generate the trees from)
        Terrace *terrace = new Terrace(params.user_file,params.is_rooted,params.pr_ab_matrix);
        //terrace->linkTrees(true,true);
        //terrace->printMapInfo();
        //terrace-> printBackMapInfo();
        
        // INITIAL TREE (idealy should be the largest subtree without unique species, a subtree of some induced partition tree)
        vector<string> taxa_names_sub;
        
        // Get a list of taxa to be inserted.
        vector<string> list_taxa_to_insert;
        
        ///// BEGIN: TESTING AUTOMATIC CHOICE FOR INITIAL TREE SELECTION AND TAXON ORDER
        terrace->matrix->getINFO_init_tree_taxon_order(taxa_names_sub,list_taxa_to_insert);
        exit(0);
        ///// END: TESTING AUTOMATIC CHOICE FOR INITIAL TREE SELECTION AND TAXON ORDER
        
        int init_part = 0;
        for(i=0; i<terrace->taxa_num; i++){
            if(terrace->matrix->pr_ab_matrix[i][init_part]==1){
                taxa_names_sub.push_back(terrace->matrix->taxa_names[i]);
                //cout<<"TAXON "<<terrace->matrix->taxa_names[i]<<endl;
            }
        }
        
        // INITIAL TERRACE (to be used for generating trees, representative is the initial tree and induced partition trees are common subtrees with induced partition trees of considered terrace from above)
        // Creating a subterrace: submatrix and an initial tree
        PresenceAbsenceMatrix *submatrix = new PresenceAbsenceMatrix();
        terrace->matrix->getSubPrAbMatrix(taxa_names_sub, submatrix);
        
        TerraceTree tree_init;
        tree_init.copyTree_byTaxonNames(terrace,taxa_names_sub);
        //tree_init.drawTree(cout, WT_BR_SCALE | WT_TAXON_ID | WT_NEWLINE);
        Terrace *init_terrace = new Terrace(tree_init, submatrix);
        
        //cout<<"taxa_names.size() = "<<init_terrace->matrix->taxa_names.size()<<endl;
        //cout<<"pr_ab_matrix.size() = "<<init_terrace->matrix->pr_ab_matrix.size()<<endl;
        //cout<<"pr_ab_matrix[i].size() = "<<init_terrace->matrix->pr_ab_matrix[0].size()<<endl;
        //init_terrace->matrix->print_pr_ab_matrix();
        
        init_terrace->linkTrees(true, false); // branch_back_map, taxon_back_map; in this case you only want to map branches
        //init_terrace->printMapInfo();
        //init_terrace-> printBackMapInfo();
        
        vector<Terrace*> part_tree_pairs;
        init_terrace->create_Top_Low_Part_Tree_Pairs(part_tree_pairs, terrace);
        /*for(i=0; i<part_tree_pairs.size(); i++){
            part_tree_pairs[i]->printMapInfo();
            part_tree_pairs[i]->printBackMapInfo();
        }*/
        
        
        
        // ORDER: no specific order, i.e. based on the input presence-absence matrix the next taxon not present on initial tree
        for(i=0; i<terrace->taxa_num; i++){
            if(init_terrace->matrix->findTaxonID(terrace->matrix->taxa_names[i])==-1){
               list_taxa_to_insert.push_back(terrace->matrix->taxa_names[i]);
            }
        }
        
        cout<<"Taxa to insert on initial tree: "<<endl;
        for(i=0; i<list_taxa_to_insert.size(); i++){
            cout<<i<<":"<<list_taxa_to_insert[i]<<endl;
        }
        
        cout<<endl<<"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++"<<endl;
        cout<<endl<<"READY TO GENERATE TREES FROM A TERRACE"<<endl;
        cout<<endl<<"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++"<<endl;
        init_terrace->print_ALL_DATA(part_tree_pairs);
        
        bool progress_status = true;
        init_terrace->generateTerraceTrees(terrace, part_tree_pairs, &list_taxa_to_insert, 0, &progress_status);
        
    } else {
        
        PresenceAbsenceMatrix matrix;
        matrix.read_pr_ab_matrix(params.pr_ab_matrix);
        matrix.print_pr_ab_matrix();
    }
    
};

