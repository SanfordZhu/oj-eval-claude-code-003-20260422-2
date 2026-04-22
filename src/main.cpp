#include <bits/stdc++.h>
using namespace std;

struct ProblemStat {
    bool solved = false;
    int wrong_before_ac = 0;     // incorrect attempts before first AC (used when solved)
    int solve_time = 0;          // time of first AC (pre-freeze or post-unfreeze)

    // Unfrozen unsolved tracking (for display "-x")
    int total_wrong = 0;         // total incorrect attempts counted when not frozen

    // Freeze tracking (for problems unsolved at freeze time)
    bool frozen = false;                 // entered frozen state
    int wrong_before_freeze = 0;         // incorrect attempts before freeze
    int submissions_after_freeze = 0;    // total submissions after freeze
    int wrong_after_freeze_total = 0;    // incorrect submissions after freeze (all)
    int wrong_after_freeze_before_ac = 0;// incorrect after freeze before first AC
    bool accepted_after_freeze = false;  // whether there is an AC after freeze
    int time_first_ac_after_freeze = 0;  // time of first AC after freeze
};

struct Team {
    string name;
    vector<ProblemStat> prob;
    int solved_cnt = 0;
    long long penalty = 0;
    vector<int> solve_times_desc;
    int last_rank = 0; // ranking after last FLUSH (1-based)
};

struct Submission {
    int time;
    int team_idx;
    int prob_idx;
    string status;
};

struct SystemState {
    bool started = false;
    int duration = 0;
    int m = 0; // problems count
    vector<Team> teams;
    unordered_map<string,int> team_id;
    vector<Submission> submissions; // all submissions
    bool frozen = false;
    int freeze_submission_index = -1; // index into submissions at freeze time
};

static int probIndex(char c){ return c - 'A'; }

static void recompute_team_metrics(Team &t){
    t.solved_cnt = 0; t.penalty = 0; t.solve_times_desc.clear();
    for(const auto &ps : t.prob){
        if(ps.solved && !ps.frozen){
            t.solved_cnt++;
            t.penalty += 20LL * ps.wrong_before_ac + ps.solve_time;
            t.solve_times_desc.push_back(ps.solve_time);
        }
    }
    sort(t.solve_times_desc.begin(), t.solve_times_desc.end(), greater<int>());
}

static int cmp_team(const Team &a, const Team &b){
    if(a.solved_cnt != b.solved_cnt) return a.solved_cnt > b.solved_cnt ? -1 : 1;
    if(a.penalty != b.penalty) return a.penalty < b.penalty ? -1 : 1;
    const auto &A = a.solve_times_desc;
    const auto &B = b.solve_times_desc;
    size_t n = max(A.size(), B.size());
    for(size_t i=0;i<n;++i){
        int av = (i<A.size()?A[i]:INT_MIN);
        int bv = (i<B.size()?B[i]:INT_MIN);
        if(av != bv) return av < bv ? -1 : 1; // smaller max solve time ranks higher
    }
    if(a.name != b.name) return a.name < b.name ? -1 : 1;
    return 0;
}

static vector<int> compute_rank_order(vector<Team> &teams){
    vector<int> idx(teams.size());
    iota(idx.begin(), idx.end(), 0);
    sort(idx.begin(), idx.end(), [&](int i, int j){
        int c = cmp_team(teams[i], teams[j]);
        if(c!=0) return c<0;
        return false;
    });
    return idx;
}

static void update_last_ranks(vector<Team> &teams, const vector<int>& order){
    for(size_t i=0;i<order.size();++i){ teams[order[i]].last_rank = (int)i+1; }
}

static void print_scoreboard(SystemState &S, const vector<int>& order){
    for(size_t pos=0; pos<order.size(); ++pos){
        int ti = order[pos];
        Team &t = S.teams[ti];
        cout << t.name << ' ' << (pos+1) << ' ' << t.solved_cnt << ' ' << t.penalty;
        for(int p=0;p<S.m;++p){
            const auto &ps = t.prob[p];
            if(ps.frozen){
                int x = ps.wrong_before_freeze;
                int y = ps.submissions_after_freeze;
                if(x==0) cout << ' ' << "0/" << y;
                else cout << ' ' << '-' << x << '/' << y;
            } else if(ps.solved){
                int x = ps.wrong_before_ac;
                if(x==0) cout << ' ' << '+';
                else cout << ' ' << '+' << x;
            } else {
                int x = ps.total_wrong;
                if(x==0) cout << ' ' << '.';
                else cout << ' ' << '-' << x;
            }
        }
        cout << '\n';
    }
}

int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    SystemState S;
    string cmd;
    bool flushed_once = false;

    while(cin >> cmd){
        if(cmd == "ADDTEAM"){
            string team_name; cin >> team_name;
            if(S.started){
                cout << "[Error]Add failed: competition has started.\n";
            } else if(S.team_id.count(team_name)){
                cout << "[Error]Add failed: duplicated team name.\n";
            } else {
                int id = (int)S.teams.size();
                Team t; t.name = team_name; S.teams.push_back(t);
                S.team_id[team_name] = id;
                cout << "[Info]Add successfully.\n";
            }
        } else if(cmd == "START"){
            string tmp; // DURATION
            int duration_time; string tmp2; // PROBLEM
            int problem_count;
            cin >> tmp >> duration_time >> tmp2 >> problem_count;
            if(S.started){
                cout << "[Error]Start failed: competition has started.\n";
            } else {
                S.started = true; S.duration = duration_time; S.m = problem_count;
                for(auto &t : S.teams){ t.prob.assign(S.m, ProblemStat()); }
                cout << "[Info]Competition starts.\n";
            }
        } else if(cmd == "SUBMIT"){
            string problem_name, tmpBY, team_name, tmpWITH, status, tmpAT; int time;
            cin >> problem_name >> tmpBY >> team_name >> tmpWITH >> status >> tmpAT >> time;
            int pi = probIndex(problem_name[0]);
            int ti = S.team_id[team_name];
            Submission sub{time, ti, pi, status};
            S.submissions.push_back(sub);
            auto &ps = S.teams[ti].prob[pi];
            if(!S.frozen){
                if(ps.solved){
                    // already solved, no effect
                } else {
                    if(status == "Accepted"){
                        ps.solved = true;
                        ps.solve_time = time;
                        // wrong_before_ac already counted
                    } else {
                        ps.wrong_before_ac++;
                        ps.total_wrong++;
                    }
                }
            } else {
                if(!ps.solved){
                    ps.frozen = true;
                    ps.submissions_after_freeze++;
                    if(status == "Accepted"){
                        if(!ps.accepted_after_freeze){
                            ps.accepted_after_freeze = true;
                            ps.time_first_ac_after_freeze = time;
                        }
                    } else {
                        ps.wrong_after_freeze_total++;
                        if(!ps.accepted_after_freeze) ps.wrong_after_freeze_before_ac++;
                    }
                } else {
                    // solved before freeze: ignore
                }
            }
        } else if(cmd == "FLUSH"){
            cout << "[Info]Flush scoreboard.\n";
            for(auto &t : S.teams) recompute_team_metrics(t);
            auto order = compute_rank_order(S.teams);
            update_last_ranks(S.teams, order);
            flushed_once = true;
        } else if(cmd == "FREEZE"){
            if(S.frozen){
                cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
            } else {
                for(auto &t : S.teams){
                    for(auto &ps : t.prob){
                        if(!ps.solved){
                            ps.wrong_before_freeze = ps.total_wrong; // incorrect before freeze
                        }
                    }
                }
                S.frozen = true;
                S.freeze_submission_index = (int)S.submissions.size();
                cout << "[Info]Freeze scoreboard.\n";
            }
        } else if(cmd == "SCROLL"){
            if(!S.frozen){
                cout << "[Error]Scroll failed: scoreboard has not been frozen.";
                continue;
            }
            cout << "[Info]Scroll scoreboard.\n";
            for(auto &t : S.teams) recompute_team_metrics(t);
            auto order = compute_rank_order(S.teams);
            print_scoreboard(S, order);

            auto hasFrozen = [&](){
                for(auto &t : S.teams){ for(auto &ps : t.prob){ if(ps.frozen) return true; } }
                return false;
            };
            auto frozen_problem_smallest = [&](Team &t){
                for(int p=0;p<S.m;++p){ if(t.prob[p].frozen) return p; }
                return -1;
            };

            // Maintain order incrementally during scroll
            while(hasFrozen()){
                int chosen_idx = -1;
                int chosen_pos = -1;
                for(int i=(int)order.size()-1;i>=0;--i){
                    int ti = order[i];
                    if(frozen_problem_smallest(S.teams[ti]) != -1){ chosen_idx = ti; chosen_pos = i; break; }
                }
                if(chosen_idx == -1) break;
                int p = frozen_problem_smallest(S.teams[chosen_idx]);
                auto &ps = S.teams[chosen_idx].prob[p];

                // Unfreeze this problem and merge status
                ps.frozen = false;
                ps.submissions_after_freeze = 0;
                if(ps.accepted_after_freeze){
                    ps.solved = true;
                    ps.solve_time = ps.time_first_ac_after_freeze;
                    ps.wrong_before_ac = ps.wrong_before_freeze + ps.wrong_after_freeze_before_ac;
                } else {
                    ps.total_wrong = ps.wrong_before_freeze + ps.wrong_after_freeze_total;
                }
                ps.wrong_after_freeze_total = 0;
                ps.wrong_after_freeze_before_ac = 0;
                ps.accepted_after_freeze = false;
                ps.time_first_ac_after_freeze = 0;

                // Recompute metrics only for chosen team
                recompute_team_metrics(S.teams[chosen_idx]);

                // Bubble up in order while beats predecessor
                int pos = chosen_pos;
                if(pos>0){
                    int prev_team_idx = order[pos-1];
                    if(cmp_team(S.teams[chosen_idx], S.teams[prev_team_idx]) < 0){
                        cout << S.teams[chosen_idx].name << ' ' << S.teams[prev_team_idx].name << ' '
                             << S.teams[chosen_idx].solved_cnt << ' ' << S.teams[chosen_idx].penalty << "\n";
                    }
                }
                while(pos>0 && cmp_team(S.teams[chosen_idx], S.teams[order[pos-1]]) < 0){
                    swap(order[pos], order[pos-1]);
                    pos--;
                }
            }

            S.frozen = false;
            print_scoreboard(S, order);

            S.frozen = false;
            print_scoreboard(S, order);
        } else if(cmd == "QUERY_RANKING"){
            string team_name; cin >> team_name;
            if(!S.team_id.count(team_name)){
                cout << "[Error]Query ranking failed: cannot find the team.\n";
            } else {
                cout << "[Info]Complete query ranking.\n";
                if(S.frozen) cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
                int ti = S.team_id[team_name];
                int rank_to_show = S.teams[ti].last_rank;
                if(!flushed_once){
                    // ranking by lexicographic order of names
                    vector<pair<string,int>> arr;
                    for(int i=0;i<(int)S.teams.size();++i) arr.push_back({S.teams[i].name,i});
                    sort(arr.begin(), arr.end());
                    for(int i=0;i<(int)arr.size();++i){ if(arr[i].second==ti){ rank_to_show = i+1; break; } }
                }
                cout << '[' << S.teams[ti].name << "] NOW AT RANKING " << rank_to_show << "\n";
            }
        } else if(cmd == "QUERY_SUBMISSION"){
            string team_name; string tmpWHERE; string tmpPROBLEM; string probSel; string tmpAND; string tmpSTATUS; string statusSel;
            cin >> team_name >> tmpWHERE >> tmpPROBLEM >> probSel >> tmpAND >> tmpSTATUS >> statusSel;
            if(!S.team_id.count(team_name)){
                cout << "[Error]Query submission failed: cannot find the team.\n";
            } else {
                cout << "[Info]Complete query submission.\n";
                int ti = S.team_id[team_name];
                int targetProb = -1;
                if(probSel != "ALL") targetProb = probIndex(probSel[0]);
                string targetStatus = statusSel;
                bool found=false; Submission last{};
                for(int i=(int)S.submissions.size()-1;i>=0;--i){
                    const auto &sub = S.submissions[i];
                    if(sub.team_idx!=ti) continue;
                    if(targetProb!=-1 && sub.prob_idx!=targetProb) continue;
                    if(targetStatus!="ALL" && sub.status!=targetStatus) continue;
                    last = sub; found=true; break;
                }
                if(!found){
                    cout << "Cannot find any submission.\n";
                } else {
                    char pc = char('A' + last.prob_idx);
                    cout << S.teams[ti].name << ' ' << pc << ' ' << last.status << ' ' << last.time << "\n";
                }
            }
        } else if(cmd == "END"){
            cout << "[Info]Competition ends.\n";
            break;
        }
    }
    return 0;
}
