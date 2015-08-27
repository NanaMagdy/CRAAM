#include <limits>
#include <algorithm>
#include <string>
#include <sstream>
#include <utility>
#include <iostream>

#include "RMDP.hpp"

namespace craam {

long RMDP::state_count() const{
    return this->states.size();
}

long RMDP::action_count(long stateid) const{
    if(stateid < 0l || stateid >= (long) this->states.size()){
        throw invalid_argument("invalid state number");
    }
    return this->states[stateid].actions.size();
}

long RMDP::outcome_count(long stateid, long actionid) const{
    if(stateid < 0l || stateid >= (long) this->states.size()){
        throw invalid_argument("invalid state number");
    }
    if(actionid < 0l || actionid >= (long) this->states[stateid].actions.size()){
        throw invalid_argument("invalid action number");
    }
    return this->states[stateid].actions[actionid].outcomes.size();
}

long RMDP::transition_count(long stateid, long actionid, long outcomeid) const{
    if(stateid < 0l || stateid >= (long) this->states.size()){
        throw invalid_argument("invalid state number");
    }
    if(actionid < 0l || actionid >= (long) this->states[stateid].actions.size()){
        throw invalid_argument("invalid action number");
    }
    if(outcomeid < 0l || outcomeid >= (long) this->states[stateid].actions[actionid].outcomes.size()){
        throw invalid_argument("invalid outcome number");
    }
    return this->states[stateid].actions[actionid].outcomes[outcomeid].indices.size();
}

long RMDP::sample_count(long stateid, long actionid, long outcomeid) const{
    /**
       Returns the number of samples (state to state transitions) for the
              given parameters.

       \param fromid Starting state ID
       \param actionid Action ID
       \param outcomeid Outcome ID (A single outcome corresponds to a regular MDP)
       \param sampleid Sample (a single state transition) ID
     */
    const Transition& tran = this->get_transition(stateid,actionid,outcomeid);
    return tran.rewards.size();
}

void RMDP::set_reward(long stateid, long actionid, long outcomeid, long sampleid, prec_t reward){
    /**
       Sets the reward for the given sample id.
       \param fromid Starting state ID
       \param actionid Action ID
       \param outcomeid Outcome ID (A single outcome corresponds to a regular MDP)
       \param sampleid Sample (a single state transition) ID
       \param reward The new reward
     */

    Transition& tran = this->get_transition(stateid,actionid,outcomeid);

    if(sampleid < 0l || sampleid >= (long) tran.rewards.size()){
        throw invalid_argument("invalid sample number");
    }
    tran.rewards[sampleid] = reward;
}


prec_t RMDP::get_reward(long stateid, long actionid, long outcomeid, long sampleid) const {
    /**
       Returns the reward for the given sample id.
       \param fromid Starting state ID
       \param actionid Action ID
       \param outcomeid Outcome ID (A single outcome corresponds to a regular MDP)
       \param sampleid Sample (a single state transition) ID
     */
    const Transition& tran = this->get_transition(stateid,actionid,outcomeid);

    if(sampleid < 0l || sampleid >= (long) tran.rewards.size()){
        throw invalid_argument("invalid sample number");
    }
    return tran.rewards[sampleid];
}

prec_t RMDP::get_toid(long stateid, long actionid, long outcomeid, long sampleid) const {
    /**
       Returns the target state for the given sample id.
       \param fromid Starting state ID
       \param actionid Action ID
       \param outcomeid Outcome ID (A single outcome corresponds to a regular MDP)
       \param sampleid Sample (a single state transition) ID
     */
    const Transition& tran = this->get_transition(stateid,actionid,outcomeid);

    if(sampleid < 0l || sampleid >= (long) tran.rewards.size()){
        throw invalid_argument("invalid sample number");
    }
    return tran.indices[sampleid];
}

prec_t RMDP::get_probability(long stateid, long actionid, long outcomeid, long sampleid) const {
    /**
       Returns the probability for the given sample id.
       \param fromid Starting state ID
       \param actionid Action ID
       \param outcomeid Outcome ID (A single outcome corresponds to a regular MDP)
       \param sampleid Sample (a single state transition) ID
     */
    const Transition& tran = this->get_transition(stateid,actionid,outcomeid);

    if(sampleid < 0l || sampleid >= (long) tran.rewards.size()){
        throw invalid_argument("invalid sample number");
    }
    return tran.probabilities[sampleid];
}

void RMDP::add_transition(long fromid, long actionid, long outcomeid, long toid, prec_t probability, prec_t reward){
    /**
       Adds a transition probability
     *
       \param fromid Starting state ID
       \param actionid Action ID
       \param outcomeid Outcome ID (A single outcome corresponds to a regular MDP)
       \param toid Destination ID
       \param probability Probability of the transition (must be non-negative)
       \param reward The reward associated with the transition.
     */

    if(fromid < 0l) throw invalid_argument("incorrect fromid");
    if(toid < 0l)   throw invalid_argument("incorrect toid");

    auto newid = max(fromid,toid);

    if(newid >= (long) this->states.size()){
        // re-sizing to accommodate the new state
        (this->states).resize(newid+1);
    }

    this->states[fromid].add_action(actionid, outcomeid, toid, probability, reward);
}

void RMDP::add_transition_d(long fromid, long actionid, long toid, prec_t probability, prec_t reward){
    /** Adds a non-robust transition.  */
    this->add_transition(fromid, actionid, 0, toid, probability, reward);
}


bool RMDP::is_normalized() const{
    /**
       Check if all transitions in the process are normalized.

       Note that if there are no actions, or no outcomes for a state, the RMDP still may be normalized.

       \return True if and only if all transitions are normalized.
     */

    for(auto const& s : states){
        for(auto const& a : s.actions){
            for(auto const& t : a.outcomes){
                if(!t.is_normalized())
                    return false;
            }
        }
    }
    return true;
}
void RMDP::normalize(){
    /**
       Normalize all transitions for all states, actions, outcomes.
     */

     for(auto& s : states){
        for(auto& a : s.actions){
            for(auto& t : a.outcomes){
                t.normalize();
            }
        }
    }
}

void RMDP::add_transitions(vector<long> const& fromids, vector<long> const& actionids, vector<long> const& outcomeids, vector<long> const& toids, vector<prec_t> const& probs, vector<prec_t> const& rews){
    /**
        Add multiple samples (transitions) to the MDP definition

       \param fromids Starting state ids
       \param outcomeis IDs used of the outcomes
       \param toids Destination state ids
       \param actionids
       \param probs Probabilities of the transitions
       \param rews Rewards of the transitions
     */

    auto s = fromids.size();
    if(s != outcomeids.size() || s != toids.size() || s != actionids.size() || s != probs.size() || s != rews.size())
        throw invalid_argument("sizes do not match.");

    for(auto l=0l; l <= (long) s; l++)
        this->add_transition(fromids[l],actionids[l],outcomeids[l],toids[l],probs[l],rews[l]);
}

void RMDP::set_distribution(long fromid, long actionid, vector<prec_t> const& distribution, prec_t threshold){
    if(fromid >= (long) this->states.size() || fromid < 0){
        throw invalid_argument("no such state");
    }
    if(actionid >= (long) states[fromid].actions.size() || actionid < 0){
        throw invalid_argument("no such action");
    }
    states[fromid].actions[actionid].set_distribution(distribution, threshold);
}

void RMDP::set_uniform_thresholds(prec_t threshold){
    /**
       Sets thresholds for all states uniformly
     */
    for(auto& s : this->states){
        s.set_thresholds(threshold);
    }
}

Transition& RMDP::get_transition(long stateid, long actionid, long outcomeid){
    /**
       Returns transition states, probabilities, and rewards
     */
    if(stateid < 0l || stateid >= (long) this->states.size()){
        throw invalid_argument("invalid state number");
    }
    if(actionid < 0l || actionid >= (long) this->states[stateid].actions.size()){
        throw invalid_argument("invalid action number");
    }
    if(outcomeid < 0l || outcomeid >= (long) this->states[stateid].actions[actionid].outcomes.size()){
        throw invalid_argument("invalid outcome number");
    }

    return (this->states[stateid].actions[actionid].outcomes[outcomeid]);
}

const Transition& RMDP::get_transition(long stateid, long actionid, long outcomeid) const{
    /**
       Returns transition states, probabilities, and rewards
     */
    if(stateid < 0l || stateid >= (long) this->states.size()){
        throw invalid_argument("invalid state number");
    }
    if(actionid < 0l || actionid >= (long) this->states[stateid].actions.size()){
        throw invalid_argument("invalid action number");
    }
    if(outcomeid < 0l || outcomeid >= (long) this->states[stateid].actions[actionid].outcomes.size()){
        throw invalid_argument("invalid outcome number");
    }

    return (this->states[stateid].actions[actionid].outcomes[outcomeid]);
}



unique_ptr<RMDP> RMDP::transitions_from_csv(istream& input, bool header){
    /**
       Loads an RMDP definition from a simple csv file

       States, actions, and outcomes are identified by 0-based ids.

       The columns are separated by commas, and rows by new lines.

       The file is formatted with the following columns:
       idstatefrom, idaction, idoutcome, idstateto, probability, reward

       Note that outcome distributions are not restored.

       \param input Source of the RMDP
       \param header Whether the first line of the file represents the header.
                        The column names are not checked for correctness or number!
     */

    string line;

    // skip the first row if so instructed
    if(header) input >> line;

    unique_ptr<RMDP> result(new RMDP());

    input >> line;
    while(input.good()){

        string cellstring;
        stringstream linestream(line);

        long idstatefrom, idstateto, idaction, idoutcome;
        prec_t probability, reward;

        // read idstatefrom
        getline(linestream, cellstring, ',');
        idstatefrom = stoi(cellstring);

        // read idaction
        getline(linestream, cellstring, ',');
        idaction = stoi(cellstring);

        // read idoutcome
        getline(linestream, cellstring, ',');
        idoutcome = stoi(cellstring);

        // read idstateto
        getline(linestream, cellstring, ',');
        idstateto = stoi(cellstring);

        // read probability
        getline(linestream, cellstring, ',');
        probability = stof(cellstring);

        // read reward
        getline(linestream, cellstring, ',');
        reward = stof(cellstring);

        result->add_transition(idstatefrom,idaction,idoutcome,idstateto,probability,reward);

        input >> line;
    }

    return result;
}

void RMDP::transitions_to_csv(ostream& output, bool header) const{
    /**
       Saves the model to a stream as a simple csv file

       States, actions, and outcomes are identified by 0-based ids.

       The columns are separated by commas, and rows by new lines.

       The file is formatted with the following columns:
       idstatefrom, idaction, idoutcome, idstateto, probability, reward

       Exported and imported MDP will be be slightly different. Since action/transitions
       will not be exported if there are no actions for the state. However, when
       there is data for action 1 and action 3, action 2 will be created with no outcomes.

       Note that outcome distributions are not saved.

       \param output Output for the stream
       \param header Whether the header should be written as the
              first line of the file represents the header.
     */

    //write header is so requested
    if(header){
        output << "idstatefrom," << "idaction," <<
            "idoutcome," << "idstateto," << "probability," << "reward" << endl;
    }

    //idstatefrom
    for(size_t i = 0l; i < this->states.size(); i++){
        const auto& actions = (this->states[i]).actions;

        //idaction
        for(size_t j = 0; j < actions.size(); j++){
            const auto& outcomes = actions[j].outcomes;

            //idoutcome
            for(size_t k = 0; k < outcomes.size(); k++){
                const auto& indices = outcomes[k].indices;
                const auto& rewards = outcomes[k].rewards;
                const auto& probabilities = outcomes[k].probabilities;

                //idstateto
                for (size_t l = 0; l < indices.size(); l++){
                    output << i << ',' << j << ',' << k << ',' << indices[l] << ','
                            << probabilities[l] << ',' << rewards[l] << endl;
                }
            }
        }
    }
}

unique_ptr<RMDP> RMDP::copy() const {
    /**
       Creates a copy of the MDP.
     */

    unique_ptr<RMDP> result(new RMDP(this->state_count()));

    this->copy_into(*result);

    return result;
}

void RMDP::copy_into(RMDP& result) const {
    /**
       Copies the values of the RMDP to a new one. The new RMDP should be empty.
     */

    // make sure that the created MDP is empty

    // *** copy transitions ***
    //idstatefrom
    for(size_t i = 0l; i < this->states.size(); i++){
        const auto& actions = (this->states[i]).actions;

        //idaction
        for(size_t j = 0; j < actions.size(); j++){
            const auto& outcomes = actions[j].outcomes;

            //idoutcome
            for(size_t k = 0; k < outcomes.size(); k++){
                const auto& indices = outcomes[k].indices;
                const auto& rewards = outcomes[k].rewards;
                const auto& probabilities = outcomes[k].probabilities;

                //idstateto
                for (size_t l = 0; l < indices.size(); l++){
                    result.add_transition(i,j,k,indices[l],probabilities[l],rewards[l]);
                }
            }
        }
    }

    // *** copy distributions and thresholds ***
    for(size_t i = 0l; i < this->states.size(); i++){
        const auto& actions_origin = (this->states[i]).actions;
        auto& actions_dest = (result.states[i]).actions;

        //idaction
        for(size_t j = 0; j < actions_origin.size(); j++){
            const auto& action_origin = actions_origin[j];
            auto& action_dest = actions_dest[j];

            action_dest.distribution = action_origin.distribution;
            action_dest.threshold = action_origin.threshold;
        }
    }
}

string RMDP::to_string() const {
    /** Returns a brief string representation of the MDP.

       This method is mostly suitable for analyzing small MDPs.

     */
    string result;

    for(size_t i = 0; i < this->states.size(); i++){
        auto s = this->states[i];
        result.append(std::to_string(i));
        result.append(" : ");
        auto actions = s.actions;
        result.append(std::to_string(actions.size()));
        result.append("\n");
        for(size_t j = 0; j < actions.size(); j++){
            result.append("    ");
            result.append(std::to_string(j));
            result.append(" : ");
            result.append(std::to_string(actions[j].outcomes.size()));
            result.append(" / ");
            result.append(std::to_string(actions[j].distribution.size()));
            result.append("\n");
        }
    }
    return result;
}

void RMDP::set_uniform_distribution(prec_t threshold){
    /**
       Sets the distribution for outcomes for each state and
       action to be uniform. It also sets the threshold to be the same
       for all states.
     */
    for(auto& s : states){
        for(auto& a : s.actions){
            auto outcomecount = a.outcomes.size();
            prec_t p = 1.0 / (prec_t) outcomecount;
            vector<prec_t> distribution(outcomecount, p);
            a.set_distribution(distribution, threshold);
        }
    }
}

void RMDP::transitions_to_csv_file(const string& filename, bool header ) const{
    /**
       Saves the transition probabilities and rewards to a CSV file

       \param filename Name of the file
       \param header Whether to create a header of the file too
     */
    ofstream ofs;
    ofs.open(filename);

    transitions_to_csv(ofs,header);
    ofs.close();
}

void RMDP::set_threshold(long stateid, long actionid, prec_t threshold){
    /**
       Sets a new threshold value
     */
    if(stateid < 0l || stateid >= (long) this->states.size()){
        throw invalid_argument("invalid state number");
    }
    if(actionid < 0l || actionid >= (long) this->states[stateid].actions.size()){
        throw invalid_argument("invalid action number");
    }
    if(threshold < 0.0 || threshold > 2.0) {
        throw invalid_argument("threshold must be between 0 and 2");
    }

    this->states[stateid].actions[actionid].threshold = threshold;
}

prec_t RMDP::get_threshold(long stateid, long actionid) const {
    /**
       Returns the threshold value
     */
    if(stateid < 0l || stateid >= (long) this->states.size()){
        throw invalid_argument("invalid state number");
    }
    if(actionid < 0l || actionid >= (long) this->states[stateid].actions.size()){
        throw invalid_argument("invalid action number");
    }

    return (this->states[stateid].actions[actionid].threshold);
}

}
