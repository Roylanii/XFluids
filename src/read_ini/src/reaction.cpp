#include <iostream>
#include "../setupini.h"

// =======================================================
// =======================================================
void Setup::ReadReactions()
{
    h_react.Nu_f_ = middle::MallocHost<int>(h_react.Nu_f_, NUM_REA * NUM_SPECIES, q);
    h_react.Nu_b_ = middle::MallocHost<int>(h_react.Nu_b_, NUM_REA * NUM_SPECIES, q);
    h_react.Nu_d_ = middle::MallocHost<int>(h_react.Nu_d_, NUM_REA * NUM_SPECIES, q);
    h_react.React_ThirdCoef = middle::MallocHost<real_t>(h_react.React_ThirdCoef, NUM_REA * NUM_SPECIES, q);
    h_react.Rargus = middle::MallocHost<real_t>(h_react.Rargus, NUM_REA * 6, q);
    h_react.react_type = middle::MallocHost<int>(h_react.react_type, NUM_REA * 2, q);
    h_react.third_ind = middle::MallocHost<int>(h_react.third_ind, NUM_REA, q);

    char Key_word[128];
    std::string rpath = WorkDir + std::string(RFile) + "/reaction_list.dat";
    std::ifstream fint(rpath);
    if (fint.good())
    {
        fint.seekg(0);
        while (!fint.eof())
        {
            fint >> Key_word;
            if (!std::strcmp(Key_word, "*forward_reaction"))
            { // stoichiometric coefficients of each reactants in all forward reactions
                for (int i = 0; i < NUM_REA; i++)
                    for (int j = 0; j < NUM_SPECIES; j++)
                        fint >> h_react.Nu_f_[i * NUM_SPECIES + j];
            }
            else if (!std::strcmp(Key_word, "*backward_reaction"))
            { // stoichiometric coefficients of each reactants in all backward reactions
                // Nu_b_ is also the coefficients of products of previous forward reactions
                for (int i = 0; i < NUM_REA; i++)
                    for (int j = 0; j < NUM_SPECIES; j++)
                    {
                        fint >> h_react.Nu_b_[i * NUM_SPECIES + j]; // the net stoichiometric coefficients
                        h_react.Nu_d_[i * NUM_SPECIES + j] = h_react.Nu_b_[i * NUM_SPECIES + j] - h_react.Nu_f_[i * NUM_SPECIES + j];
                    }
            }
            else if (!std::strcmp(Key_word, "*third_body"))
            { // the third body coefficients
                for (int i = 0; i < NUM_REA; i++)
                    for (int j = 0; j < NUM_SPECIES; j++)
                        fint >> h_react.React_ThirdCoef[i * NUM_SPECIES + j];
            }
            else if (!std::strcmp(Key_word, "*Arrhenius"))
            { // reaction rate constant parameters, A, B, E for Arrhenius law
                for (int i = 0; i < NUM_REA; i++)
                {
                    fint >> h_react.Rargus[i * 6 + 0] >> h_react.Rargus[i * 6 + 1] >> h_react.Rargus[i * 6 + 2];
                    if (BackArre)
                        fint >> h_react.Rargus[i * 6 + 3] >> h_react.Rargus[i * 6 + 4] >> h_react.Rargus[i * 6 + 5];
                } //-----------------*backwardArrhenius------------------//
            }
        }
    }
    fint.close();

    IniSpeciesReactions();

    {
        d_react.Nu_f_ = middle::MallocDevice<int>(d_react.Nu_f_, NUM_REA * NUM_SPECIES, q);
        d_react.Nu_b_ = middle::MallocDevice<int>(d_react.Nu_b_, NUM_REA * NUM_SPECIES, q);
        d_react.Nu_d_ = middle::MallocDevice<int>(d_react.Nu_d_, NUM_REA * NUM_SPECIES, q);
        d_react.react_type = middle::MallocDevice<int>(d_react.react_type, NUM_REA * 2, q);
        d_react.third_ind = middle::MallocDevice<int>(d_react.third_ind, NUM_REA, q);
        d_react.React_ThirdCoef = middle::MallocDevice<real_t>(d_react.React_ThirdCoef, NUM_REA * NUM_SPECIES, q);
        d_react.Rargus = middle::MallocDevice<real_t>(d_react.Rargus, NUM_REA * 6, q);

        middle::MemCpy<int>(d_react.Nu_f_, h_react.Nu_f_, NUM_REA * NUM_SPECIES, q);
        middle::MemCpy<int>(d_react.Nu_b_, h_react.Nu_b_, NUM_REA * NUM_SPECIES, q);
        middle::MemCpy<int>(d_react.Nu_d_, h_react.Nu_d_, NUM_REA * NUM_SPECIES, q);
        middle::MemCpy<int>(d_react.react_type, h_react.react_type, NUM_REA * 2, q);
        middle::MemCpy<int>(d_react.third_ind, h_react.third_ind, NUM_REA, q);
        middle::MemCpy<real_t>(d_react.React_ThirdCoef, h_react.React_ThirdCoef, NUM_REA * NUM_SPECIES, q);
        middle::MemCpy<real_t>(d_react.Rargus, h_react.Rargus, NUM_REA * 6, q);

        int reaction_list_size = 0;
        h_react.rns = middle::MallocHost<int>(h_react.rns, NUM_SPECIES, q);
        for (size_t i = 0; i < NUM_SPECIES; i++)
            h_react.rns[i] = reaction_list[i].size(), reaction_list_size += h_react.rns[i];

        int *h_reaction_list, *d_reaction_list;
        h_reaction_list = middle::MallocHost<int>(h_reaction_list, reaction_list_size, q);
        h_react.reaction_list = middle::MallocHost2D<int>(h_reaction_list, NUM_SPECIES, h_react.rns, q);
        for (size_t i = 0; i < NUM_SPECIES; i++)
            if (h_react.rns[i] > 0)
                std::memcpy(h_react.reaction_list[i], &(reaction_list[i][0]), sizeof(int) * h_react.rns[i]);
        d_reaction_list = middle::MallocDevice<int>(d_reaction_list, reaction_list_size, q);
        middle::MemCpy<int>(d_reaction_list, h_reaction_list, reaction_list_size, q);
        d_react.reaction_list = middle::MallocDevice2D<int>(d_reaction_list, NUM_SPECIES, h_react.rns, q);

        h_react.rts = middle::MallocHost<int>(h_react.rts, NUM_REA, q);
        h_react.pls = middle::MallocHost<int>(h_react.pls, NUM_REA, q);
        h_react.sls = middle::MallocHost<int>(h_react.sls, NUM_REA, q);
        int rts_size = 0, pls_size = 0, sls_size = 0;
        for (size_t i = 0; i < NUM_REA; i++)
        {
            h_react.rts[i] = reactant_list[i].size(), rts_size += h_react.rts[i];
            h_react.pls[i] = product_list[i].size(), pls_size += h_react.pls[i];
            h_react.sls[i] = species_list[i].size(), sls_size += h_react.sls[i];
        }
        d_react.rns = middle::MallocDevice<int>(d_react.rns, NUM_SPECIES, q);
        d_react.rts = middle::MallocDevice<int>(d_react.rts, NUM_REA, q);
        d_react.pls = middle::MallocDevice<int>(d_react.pls, NUM_REA, q);
        d_react.sls = middle::MallocDevice<int>(d_react.sls, NUM_REA, q);
        middle::MemCpy<int>(d_react.rns, h_react.rns, NUM_SPECIES, q);
        middle::MemCpy<int>(d_react.rts, h_react.rts, NUM_REA, q);
        middle::MemCpy<int>(d_react.pls, h_react.pls, NUM_REA, q);
        middle::MemCpy<int>(d_react.sls, h_react.sls, NUM_REA, q);

        int *h_reactant_list, *h_product_list, *h_species_list, *d_reactant_list, *d_product_list, *d_species_list;
        h_reactant_list = middle::MallocHost<int>(h_reactant_list, rts_size, q);
        h_product_list = middle::MallocHost<int>(h_product_list, pls_size, q);
        h_species_list = middle::MallocHost<int>(h_species_list, sls_size, q);
        h_react.reactant_list = middle::MallocHost2D<int>(h_reactant_list, NUM_REA, h_react.rts, q);
        h_react.product_list = middle::MallocHost2D<int>(h_product_list, NUM_REA, h_react.pls, q);
        h_react.species_list = middle::MallocHost2D<int>(h_species_list, NUM_REA, h_react.sls, q);

        for (size_t i = 0; i < NUM_REA; i++)
        {
            std::memcpy(h_react.reactant_list[i], &(reactant_list[i][0]), sizeof(int) * h_react.rts[i]);
            std::memcpy(h_react.product_list[i], &(product_list[i][0]), sizeof(int) * h_react.pls[i]);
            std::memcpy(h_react.species_list[i], &(species_list[i][0]), sizeof(int) * h_react.sls[i]);
        }

        d_reactant_list = middle::MallocDevice<int>(d_reactant_list, rts_size, q);
        d_product_list = middle::MallocDevice<int>(d_product_list, pls_size, q);
        d_species_list = middle::MallocDevice<int>(d_species_list, sls_size, q);
        middle::MemCpy<int>(d_reactant_list, h_reactant_list, rts_size, q);
        middle::MemCpy<int>(d_product_list, h_product_list, pls_size, q);
        middle::MemCpy<int>(d_species_list, h_species_list, sls_size, q);
        d_react.reactant_list = middle::MallocDevice2D<int>(d_reactant_list, NUM_REA, h_react.rts, q);
        d_react.product_list = middle::MallocDevice2D<int>(d_product_list, NUM_REA, h_react.pls, q);
        d_react.species_list = middle::MallocDevice2D<int>(d_species_list, NUM_REA, h_react.sls, q);
    }
}

// =======================================================
// =======================================================
void Setup::IniSpeciesReactions()
{
    for (size_t j = 0; j < NUM_SPECIES; j++)
    {
        reaction_list[j].clear();
        for (size_t i = 0; i < NUM_REA; i++)
            if (h_react.Nu_f_[i * NUM_SPECIES + j] > 0 || h_react.Nu_b_[i * NUM_SPECIES + j] > 0)
                reaction_list[j].push_back(i);
    }

    if (0 == myRank)
        printf("\n%d Reactions been actived                                     \n", NUM_REA);
    bool react_error = false;
    for (size_t i = 0; i < NUM_REA; i++)
    {
        reactant_list[i].clear();
        product_list[i].clear();
        species_list[i].clear();
        real_t sum = _DF(0.0);
        for (size_t j = 0; j < NUM_SPECIES; j++)
        {
            if (h_react.Nu_f_[i * NUM_SPECIES + j] > 0)
                reactant_list[i].push_back(j);
            if (h_react.Nu_b_[i * NUM_SPECIES + j] > 0)
                product_list[i].push_back(j);
            if (h_react.Nu_f_[i * NUM_SPECIES + j] > 0 || h_react.Nu_b_[i * NUM_SPECIES + j] > 0)
                species_list[i].push_back(j);
            // third body indicator
            sum += h_react.React_ThirdCoef[i * NUM_SPECIES + j];
        }
        h_react.third_ind[i] = (sum > _DF(0.0)) ? 1 : 0;

        if (0 == myRank)
        { // output this reaction kinetic
            if (i + 1 < 10)
                std::cout << "Reaction:" << i + 1 << "  ";
            else
                std::cout << "Reaction:" << i + 1 << " ";
            int numreactant = 0, numproduct = 0;
            // // output reactant
            for (int j = 0; j < NUM_SPECIES; ++j)
                numreactant += h_react.Nu_f_[i * NUM_SPECIES + j];

            if (numreactant == 0)
                std::cout << "0	  <-->  ";
            else
            {
                for (int j = 0; j < NUM_SPECIES; j++)
                {
                    if (h_react.Nu_f_[i * NUM_SPECIES + j] > 0)
                        std::cout << h_react.Nu_f_[i * NUM_SPECIES + j] << " " << species_name[j] << " + ";

                    if (NUM_COP == j)
                    {
                        if (h_react.React_ThirdCoef[i * NUM_SPECIES + j] > _DF(0.0))
                            std::cout << " M ";
                        std::cout << "  <-->  ";
                    }
                }
            }
            // // output product
            for (int j = 0; j < NUM_SPECIES; ++j)
                numproduct += h_react.Nu_b_[i * NUM_SPECIES + j];
            if (numproduct == 0)
                std::cout << "0	";
            else
            {
                for (int j = 0; j < NUM_SPECIES; j++)
                {
                    if (h_react.Nu_b_[i * NUM_SPECIES + j] > 0)
                        std::cout << h_react.Nu_b_[i * NUM_SPECIES + j] << " " << species_name[j] << " + ";

                    if (NUM_COP == j)
                    {
                        if (h_react.React_ThirdCoef[i * NUM_SPECIES + j] > _DF(0.0))
                            std::cout << " M ";
                    }
                }
            }
            std::cout << " with forward rate: " << h_react.Rargus[i * 6 + 0] << " " << h_react.Rargus[i * 6 + 1] << " " << h_react.Rargus[i * 6 + 2];
            //-----------------*backwardArrhenius------------------//
            if (BackArre)
                std::cout << ", backward rate: " << h_react.Rargus[i * 6 + 3] << " " << h_react.Rargus[i * 6 + 4] << " " << h_react.Rargus[i * 6 + 5];
            std::cout << std::endl;

            if (1 == h_react.third_ind[i])
            {
                std::cout << " Third Body coffcients:";
                for (int j = 0; j < NUM_SPECIES - 1; j++)
                    std::cout << "  " << species_name[j] << ":" << h_react.React_ThirdCoef[i * NUM_SPECIES + j];
                std::cout << "\n";
            }
        }

        react_error || ReactionType(0, i, h_react.Nu_f_, h_react.Nu_b_); // forward reaction kinetic
        react_error || ReactionType(1, i, h_react.Nu_b_, h_react.Nu_f_); // backward reaction kinetic
    }

    // if (react_error)
    //     exit(EXIT_FAILURE);
}

// =======================================================
// =======================================================
/**
 * @brief determine types of reaction:
 * 		1: "0 -> A";		2: "A -> 0";		    3: "A -> B";		    4: "A -> B + C";
 * 	    5: "A -> A + B"     6: "A + B -> 0"		    7: "A + B -> C"		    8: "A + B -> C + D"
 *      9: "A + B -> A";	10: "A + B -> A + C"    11: "2A -> B"		    12: "A -> 2B"
 *      13: "2A -> B + C"	14: "A + B -> 2C"	    15: "2A + B -> C + D"   16: "2A -> 2C + D"
 * @note  only a few simple reaction has analytical solution, for other cases one can use quasi-statical-state-approxiamte(ChemeQ2)
 * @param i: index of the reaction
 * @param flag: 0: forward reaction; 1: backward reaction
 * @param Nuf,Nub: forward and backward reaction matrix
 * @return true: has unsupported reaction kinetic type
 * @return false: all supported reaction kinetic type
 */
bool Setup::ReactionType(int flag, int i, int *Nuf, int *Nub)
{
    std::string error_str;
    std::vector<int> forward_list, backward_list;
    if (flag == 0)
    {
        error_str = "forward ";
        forward_list = reactant_list[i], backward_list = product_list[i];
    }
    else
    {
        error_str = "backward";
        forward_list = product_list[i], backward_list = reactant_list[i];
    }

    /**
     * @brief: reaction type
     * @note: the support is not complete
     */
    h_react.react_type[i * 2 + flag] = 0;
    int Od_Rec = 0, Od_Pro = 0, Num_Repeat = 0; // // the order of the reaction
    // // loop all species in reaction "i"
    for (int l = 0; l < forward_list.size(); l++)
    {
        int species_id = forward_list[l];
        Od_Rec += Nuf[i * NUM_SPECIES + species_id];
    }
    for (int l = 0; l < backward_list.size(); l++)
    {
        int specie_id = backward_list[l];
        Od_Pro += Nub[i * NUM_SPECIES + specie_id];
    }
    for (int l = 0; l < forward_list.size(); l++)
    {
        int specie_id = forward_list[l];
        for (int l1 = 0; l1 < backward_list.size(); l1++)
        {
            int specie2_id = backward_list[l1];
            if (specie_id == specie2_id)
            {
                Num_Repeat++;
                break;
            }
        }
    }
    // // get reaction type
    switch (Od_Rec)
    {
    case 0: // // 0th-order
        h_react.react_type[i * 2 + flag] = 1;
        break;
    case 1: // // 1st-order
        if (Od_Pro == 0)
            h_react.react_type[i * 2 + flag] = 2;
        else if (Od_Pro == 1)
            h_react.react_type[i * 2 + flag] = 3;
        else if (Od_Pro == 2)
        {
            h_react.react_type[i * 2 + flag] = Num_Repeat == 0 ? 4 : 5;
            if (Nub[i * NUM_SPECIES + backward_list[0]] == 2)
                h_react.react_type[i * 2 + flag] = 12;
        }
        break;
    case 2: // // 2nd-order
        if (Od_Pro == 0)
            h_react.react_type[i * 2 + flag] = 6;
        else if (Od_Pro == 1)
        {
            h_react.react_type[i * 2 + flag] = Num_Repeat == 0 ? 7 : 9;
            if (Nuf[i * NUM_SPECIES + forward_list[0]] == 2)
                h_react.react_type[i * 2 + flag] = 11;
        }
        else if (Od_Pro == 2)
        {
            h_react.react_type[i * 2 + flag] = Num_Repeat == 0 ? 8 : 10;
            if (Nuf[i * NUM_SPECIES + forward_list[0]] == 2)
                h_react.react_type[i * 2 + flag] = 13;
            if (Nub[i * NUM_SPECIES + backward_list[0]] == 2)
                h_react.react_type[i * 2 + flag] = 14;
        }
        else if (Od_Pro == 3)
        {
            if ((Nub[i * NUM_SPECIES + backward_list[0]] == 2) && !(Num_Repeat))
                h_react.react_type[i * 2 + flag] = 16;
        }
        break;
    case 3: // // 3rd-order
        if (Od_Pro == 2)
        {
            if (Nuf[i * NUM_SPECIES + forward_list[0]] == 2 || Nuf[i * NUM_SPECIES + forward_list[1]] == 2 && Nuf[i * NUM_SPECIES + backward_list[0]] == 1 && !(Num_Repeat))
                h_react.react_type[i * 2 + flag] = 15;
        }
        break;
    }
    if (h_react.react_type[i * 2 + flag] == 0)
    {
        if (myRank == 0)
            std::cout << " Note:no analytical solutions for" << error_str << " reaction of the " << i + 1 << " kinetic.\n";

        return true;
    }

    return false;
}
