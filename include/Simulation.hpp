#include<utility>
#include<vector>
#include<memory>
#include<random>

#include "definitions.hpp"

using namespace std;


template <class DecState,class ExpState>
struct ExpSample {
    /**
     * \brief Represents the transition from an expectation state to a
     * a decision state.
     */
    const ExpState expstate_from;
    const DecState decstate_to;
    const prec_t reward;
    const prec_t weight;
    const long step;
    const long run;

    ExpSample(const ExpState& expstate_from, const DecState& decstate_to,
              prec_t reward, prec_t weight, long step, long run):
        expstate_from(expstate_from), decstate_to(decstate_to),
        reward(reward), weight(weight), step(step), run(run)
                  {};
};

template <class DecState,class Action,class ExpState=pair<DecState,Action>>
struct DecSample {
    /**
     * \brief Represents the transition from a decision state to an
     * expectation state.
     */
    const DecState decstate_from;
    const Action action;
    const ExpState expstate_to;
    const long step;
    const long run;

    DecSample(const DecState& decstate_from, const Action& action,
              const ExpState& expstate_to, long step, long run):
        decstate_from(decstate_from), action(action),
        expstate_to(expstate_to), step(step), run(run)
        {};
};

template <class DecState,class Action,class ExpState=pair<DecState,Action>>
class Samples {
    /**
     * \brief General representation of samples
     */

public:
    vector<DecSample<DecState,Action,ExpState>> decsamples;
    vector<DecState> initial;
    vector<ExpSample<DecState,ExpState>> expsamples;

public:

    void add_dec(const DecSample<DecState,Action,ExpState>& decsample){
        /**
        * \brief Adds a sample starting in a decision state
        */

        this->decsamples.push_back(decsample);
    };

    void add_initial(const DecState& decstate){
        /**
         * \brief Adds an initial state
         *
         */
         this->initial.push_back(decstate);

    };

    void add_exp(const ExpSample<DecState,ExpState>& expsample){
        /**
         * \brief Adds a sample starting in an expectation state
         */

        this->expsamples.push_back(expsample);
    }

};


/*
 * Signature of static methods required for the simulator
 *
 * DState init_state()
 * EState transition_dec(const DState&, const Action&)
 * pair<double,DState> transition_exp(const EState&)
 * bool end_condition(const DState&)
 * vector<Action> actions(const DState&)  // needed for a random policy and value function policy
 */

template<class DState,class Action,class EState,class Simulator,Action (*policy)(DState)>
unique_ptr<Samples<DState,Action,EState>>
simulate_stateless(long horizon,long runs,prec_t prob_term=0.0,long tran_limit=-1){
    /** \brief Runs the simulator and generates samples. A simulator with no state
     *
     * \param sim Simulator that holds the state of the process
     * \param horizon Number of steps
     * \param prob_term The probability of termination in each step
     * \return Samples
     */

    unique_ptr<Samples<DState,Action,EState>> samples(new Samples<DState,Action,EState>());

    long transitions = 0;

    // initialize random numbers when appropriate
    default_random_engine generator;
    uniform_real_distribution<double> distribution(0.0,1.0);


    for(auto run=0l; run < runs; run++){

        DState&& decstate = Simulator::init_state();
        samples->add_initial(decstate);

        for(auto step=0l; step < horizon; step++){
            if(Simulator::end_condition(decstate))
                break;
            if(tran_limit > 0 && transitions > tran_limit)
                break;

            Action&& action = policy(decstate);
            EState&& expstate = Simulator::transition_dec(decstate,action);


            samples->add_dec(DecSample<DState,Action,EState>
                                (decstate, action, expstate, step, run));

            auto&& rewardstate = Simulator::transition_exp(expstate);

            auto reward = rewardstate.first;
            decstate = rewardstate.second;

            samples->add_exp(ExpSample<DState,EState>(expstate, decstate, reward, 1.0, step, run));

            // test the termination probability only after at least one transition
            if(prob_term > 0.0){
                if( distribution(generator) <= prob_term)
                    break;
            }
            transitions++;
        };

        if(tran_limit > 0 && transitions > tran_limit)
            break;
    }

    return samples;
};
