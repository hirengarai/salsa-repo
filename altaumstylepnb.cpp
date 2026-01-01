/*
 * REFERENCE IMPLEMENTATION OF Aumasson PNB searching programe where the data is divided into threads not the key words
 *
 *
 * created: 10/05/25
 * updated: 01/01/26
 *
 * by Hiren
 * Research fellow
 * NTU, Singapore
 *
 * Synopsis:
 * This file contains the PNB searching programe for the stream cipher salsa but here threads are not divided into keywords,
 * rather the processed data is divided into threads.
 *
 * CLI:
 *   g++ -std=c++2c -O3 -flto filename.cpp -o output && ./output nm log
 *
 * Needs: commonutility.hpp, salsa.hpp, pnbutility.hpp
 */

#include "header/salsa.hpp" // salsa round functions
#include <algorithm>
#include <cctype>
#include <cmath>               // pow function
#include <cstring>             // string
#include <ctime>               // time
#include <filesystem>
#include <fstream> // storing output in a file
#include <future>  // multithreading
#include <iomanip> // decimal numbers upto certain places
#include <thread>  // multithreading

using namespace std;

config::CipherInfo basic_config;
config::DLInfo diff_config;
config::SamplesInfo samples_config;
pnbinfo::PNBdetails pnb_config;

using BiasEntry = pair<u16, double>;

double matchcount(int key_bit, int key_word);
inline bool skip_this(u16 idx, const vector<u16> &skip_bits);

static atomic<u64> progress{0};

struct RunInfo
{
    u16 key_count = 0;
    u64 total_work = 0;
    vector<u16> skip_bits;
};

struct SearchResults
{
    vector<BiasEntry> pnbs;
    vector<BiasEntry> nonpnbs;
};

static void parse_cli(int argc, char *argv[], bool &show_segments)
{
    if (argc >= 2)
    {
        try
        {
            pnb_config.neutrality_measure = std::stod(argv[1]);
            if (pnb_config.neutrality_measure < 0.0 || pnb_config.neutrality_measure > 1.0)
            {
                std::cerr << "Neutrality must be in [0,1]. Using default 0.35.\n";
                pnb_config.neutrality_measure = 0.35;
            }
        }
        catch (...)
        {
            std::cerr << "Invalid neutrality input. Using default 0.35.\n";
            pnb_config.neutrality_measure = 0.35;
        }
    }

    if (argc >= 3)
    {
        for (int i = 2; i < argc; ++i)
        {
            std::string flag = argv[i];
            std::transform(flag.begin(), flag.end(), flag.begin(),
                           [](unsigned char c)
                           { return static_cast<char>(std::tolower(c)); });

            if (flag == "log" || flag == "1")
                basic_config.logfile_flag = true;
            else if (flag == "seg" || flag == "segment" || flag == "segments")
                show_segments = true;
        }
    }
}

static RunInfo init_config_and_banner(std::stringstream &dmsg)
{
    RunInfo info;

    basic_config.cipher_name = "salsa";
    basic_config.mode = "PNBsearch"; // input something useful without gap
    basic_config.word_size_bits = 32;
    basic_config.key_size = 256;
    basic_config.comment = "last round modified";
    basic_config.total_rounds = 7;

    diff_config.distinguishing_round = 5;
    diff_config.id = {{7, 31}};
    diff_config.mask = {{4, 7}};

    samples_config.samples_per_thread = 1ULL << 18;
    samples_config.samples_per_batch =
        samples_config.samples_per_thread * samples_config.max_num_threads;

    info.key_count =
        (basic_config.key_size == 128) ? KEYWORD_COUNT - 4 : KEYWORD_COUNT;

    info.total_work = static_cast<u64>(info.key_count) * WORD_SIZE;

    // optional: if you want total_samples() in showInfo to be "global experiments":
    // samples_config.num_batches = static_cast<std::size_t>(key_count) * WORD_SIZE;

    display::showInfo(&basic_config, &diff_config, &samples_config, dmsg);
    pnbinfo::showPNBConfig(pnb_config, dmsg);

    info.skip_bits = {
        // example:
        // 2, 5, 48, 74, ...
    };
    sort(info.skip_bits.begin(), info.skip_bits.end());

    return info;
}

static SearchResults run_search(const RunInfo &info)
{
    SearchResults results;
    results.pnbs.reserve(256);
    results.nonpnbs.reserve(256);

    vector<BiasEntry> temp_pnb;
    vector<BiasEntry> temp_non_pnb;
    temp_pnb.reserve(256);
    temp_non_pnb.reserve(256);

    double sum = 0.0;
    double bias = 0.0;

    vector<std::future<double>> future_results;
    future_results.reserve(samples_config.max_num_threads);

    progress.store(0, std::memory_order_relaxed);

    #ifdef SPINNER_WITH_ETA_AVAILABLE
    SpinnerWithETA spinner("Searching PNBs ...", &progress, info.total_work);
    // cout << "\n";
    spinner.start();
    #endif

    // ---------------- key-word / key-bit loop -----------------
    for (size_t key_word{0}; key_word < info.key_count; ++key_word)
    {
        for (size_t key_bit{0}; key_bit < WORD_SIZE; key_bit++)
        {
            u16 global_idx = static_cast<u16>(key_word * WORD_SIZE + key_bit);

            if (skip_this(global_idx, info.skip_bits))
                continue; // completely ignored and nothing is printed

            sum = 0.0;
            future_results.clear();

            // ---------------- launch threads for this (key_word, key_bit) -----------------
            for (u16 thread_number{0}; thread_number < samples_config.max_num_threads; ++thread_number)
                future_results.emplace_back(async(launch::async, matchcount, static_cast<int>(key_bit), static_cast<int>(key_word)));

            try
            {
                for (auto &f : future_results)
                    sum += f.get();
            }
            catch (const exception &e)
            {
                cerr << "Thread error: " << e.what() << "\n";
            }

            // samples_per_batch = samples_per_thread * max_num_threads
            bias = (2.0 * sum / static_cast<double>(samples_config.samples_per_batch)) - 1.0;

            if (std::fabs(bias) >= pnb_config.neutrality_measure && std::fabs(bias) > 0.0)
                temp_pnb.push_back({global_idx, bias});
            else
                temp_non_pnb.push_back({global_idx, bias});

            progress.fetch_add(1, std::memory_order_relaxed);
        }

        for (auto &l : temp_pnb)
            results.pnbs.push_back(l);

        for (auto &l : temp_non_pnb)
            results.nonpnbs.push_back(l);

        temp_pnb.clear();
        temp_non_pnb.clear();
    }

    // ---------------- deduplicate + sort PNB and non-PNB lists -----------------
    auto sort_by_index = [](auto &v)
    {
        std::sort(v.begin(), v.end(),
                  [](const auto &a, const auto &b)
                  { return a.first < b.first; });
        v.erase(std::unique(v.begin(), v.end(),
                            [](const auto &x, const auto &y)
                            { return x.first == y.first; }),
                v.end());
    };

    sort_by_index(results.pnbs);
    sort_by_index(results.nonpnbs);

    #ifdef SPINNER_WITH_ETA_AVAILABLE
    spinner.stop();
    #endif

    return results;
}

static vector<u16> build_sorted_indices(const vector<BiasEntry> &entries)
{
    vector<u16> indices;
    indices.reserve(entries.size());
    for (auto &e : entries)
        indices.push_back(e.first);
    return indices;
}

static void print_console_summary(const vector<u16> &pnbs_sorted_by_index,
                                  const vector<u16> &nonpnbs_sorted_by_index,
                                  bool show_segments)
{
    cout << "\n";
    // cout << basic_config.col_sep;
    cout << pnbs_sorted_by_index.size() << " PNBs (sorted by index):\n{";
    for (std::size_t i{0}; i < pnbs_sorted_by_index.size(); ++i)
    {
        cout << pnbs_sorted_by_index[i];
        if (i + 1 != pnbs_sorted_by_index.size())
            cout << ", ";
    }
    cout << "}\n";
    cout << basic_config.col_sep;

    if (show_segments)
    {
        pnbinfo::print_per_keyword_pnb_segments(pnbs_sorted_by_index, &basic_config, cout);
        pnbinfo::print_per_keyword_nonpnb_segments(nonpnbs_sorted_by_index, &basic_config, cout);
    }
}

static void write_log_if_enabled(const vector<BiasEntry> &all_pnbs,
                                 const vector<BiasEntry> &all_nonpnbs,
                                 const vector<u16> &pnbs_sorted_by_index,
                                 const vector<u16> &nonpnbs_sorted_by_index,
                                 const string &folder,
                                 Timer &timer,
                                 stringstream &dmsg)
{
    if (!basic_config.logfile_flag)
        return;

    // PNB sorted by |bias| (descending)
    std::vector<u16> pnbs_sorted_by_bias;
    {
        std::vector<std::pair<u16, double>> tmp = all_pnbs;
        std::sort(tmp.begin(), tmp.end(),
                  [](const auto &a, const auto &b)
                  {
                      return std::fabs(a.second) > std::fabs(b.second);
                  });
        for (auto &e : tmp)
            pnbs_sorted_by_bias.push_back(e.first);
    }

    // per-bit biases (size = 256 always)
    std::vector<double> bias_per_bit(256, 0.0);
    for (auto &e : all_pnbs)
        bias_per_bit[e.first] = e.second;
    for (auto &e : all_nonpnbs)
        bias_per_bit[e.first] = e.second;

    pnbinfo::print_full_pnb_report_tail(
        pnbs_sorted_by_index,
        pnbs_sorted_by_bias,
        nonpnbs_sorted_by_index,
        bias_per_bit,
        basic_config,
        dmsg);

    pnbinfo::print_per_keyword_ps_map(pnbs_sorted_by_index, nonpnbs_sorted_by_index, &basic_config, dmsg);

    dmsg << timer.end_message();

    // ---------------- save file -----------------
    std::string filename = makeLogFilename(basic_config, diff_config, &pnb_config, folder);
    std::ofstream fout(filename);
    if (fout.is_open())
    {
        fout << dmsg.str();
        fout.close();
        std::cout << "Log saved to: " << filename << "\n";
    }
    else
    {
        std::cerr << "ERROR: Could not write log file: " << filename << "\n";
    }
}

// ---------------- main function -----------------
int main(int argc, char *argv[])
{
    bool show_segments = false;
    parse_cli(argc, argv, show_segments);

    Timer timer;

    stringstream dmsg; //  log buffer
    string folder = "otheraum";

    dmsg << timer.start_message();

    // ---------------- config -----------------
    RunInfo info = init_config_and_banner(dmsg);

    cout << dmsg.str() << std::flush;
    // ---------------- config end -----------------

    SearchResults results = run_search(info);

    std::vector<u16> pnbs_sorted_by_index = build_sorted_indices(results.pnbs);
    std::vector<u16> nonpnbs_sorted_by_index = build_sorted_indices(results.nonpnbs);

    print_console_summary(pnbs_sorted_by_index, nonpnbs_sorted_by_index, show_segments);

    write_log_if_enabled(results.pnbs,
                         results.nonpnbs,
                         pnbs_sorted_by_index,
                         nonpnbs_sorted_by_index,
                         folder,
                         timer,
                         dmsg);

    cout << timer.end_message();
    return 0;
}

// ---------------- worker: match count for one (key_word, key_bit) -----------------
double matchcount(int key_bit, int key_word)
{
    salsa::InitKey init_key;
    u64 thread_match_count{0};

    u32 x0[STATEWORD_COUNT], strdx0[STATEWORD_COUNT], key[KEYWORD_COUNT],
        dx0[STATEWORD_COUNT], dstrdx0[STATEWORD_COUNT],
        DiffState[STATEWORD_COUNT], sumstate[STATEWORD_COUNT],
        minusstate[STATEWORD_COUNT], dsumstate[STATEWORD_COUNT],
        dminusstate[STATEWORD_COUNT];

    u8 fwd_parity, bwd_parity;

    const int rounded_total_rounds = basic_config.roundedTotalRounds();
    const int rounded_fwd_rounds = diff_config.roundedFwdRounds();
    const bool rounded_total_rounds_are_odd = (rounded_total_rounds % 2 != 0);
    const bool rounded_fwd_rounds_are_odd = (rounded_fwd_rounds % 2 != 0);
    const bool fwd_rounds_are_fractional = diff_config.fwdRoundsAreFractional();

    int fwd_post_round =
        fwd_rounds_are_fractional ? rounded_fwd_rounds + 2 : rounded_fwd_rounds + 1;
    int bwd_round =
        fwd_rounds_are_fractional ? rounded_fwd_rounds + 1 : rounded_fwd_rounds;

    size_t spt = samples_config.samples_per_thread;

    for (size_t loop{0}; loop < spt; ++loop)
    {
        fwd_parity = 0;
        bwd_parity = 0;

        // ---------------- salsa setup -----------------
        salsa::init_iv_const(x0);
        if (basic_config.key_size == 128)
            init_key.key_128bit(key);
        else
            init_key.key_256bit(key);

        salsa::insert_key(x0, key);

        ops::copyState(strdx0, x0);
        ops::copyState(dx0, x0);

        // ---------------- inject diff -----------------
        for (const auto &d : diff_config.id)
            TOGGLE_BIT(dx0[d.first], d.second);
        ops::copyState(dstrdx0, dx0);

        // ---------------- forward round -----------------
        for (int i{1}; i <= rounded_fwd_rounds; ++i)
        {
            frward.RoundFunction(x0, i);
            frward.RoundFunction(dx0, i);
        }
        if (fwd_rounds_are_fractional)
        {
            if (rounded_fwd_rounds_are_odd)
            {
                frward.Half_1_EvenRF(x0);
                frward.Half_1_EvenRF(dx0);
            }
            else
            {
                frward.Half_1_OddRF(x0);
                frward.Half_1_OddRF(dx0);
            }
        }

        // ---------------- XOR state -----------------
        ops::xorState(x0, dx0, DiffState);

        // ---------------- store forward parity -----------------
        for (const auto &d : diff_config.mask)
            fwd_parity ^= GET_BIT(DiffState[d.first], d.second);

        // ---------------- forward round -----------------
        if (fwd_rounds_are_fractional)
        {
            if (rounded_fwd_rounds_are_odd)
            {
                frward.Half_2_EvenRF(x0);
                frward.Half_2_EvenRF(dx0);
            }
            else
            {
                frward.Half_2_OddRF(x0);
                frward.Half_2_OddRF(dx0);
            }
        }

        for (int i{fwd_post_round}; i <= rounded_total_rounds; ++i)
        {
            frward.RoundFunction(x0, i);
            frward.RoundFunction(dx0, i);
        }

        if (basic_config.totalRoundsAreFractional())
        {
            if (rounded_total_rounds_are_odd)
            {
                frward.Half_1_EvenRF(x0);
                frward.Half_1_EvenRF(dx0);
            }
            else
            {
                frward.Half_1_OddRF(x0);
                frward.Half_1_OddRF(dx0);
            }
        }

        // 7.5 rounds
        frward.Half_1_EvenRF(x0);
        frward.Half_1_EvenRF(dx0);

        qr.EVENARX_13(x0);
        qr.EVENARX_13(dx0);

        qr.UEVENARX_18(x0);
        qr.UEVENARX_18(dx0);

        // ---------------- forward round end -----------------

        // ---------------- Z = X + X^R -----------------
        ops::addState(x0, strdx0, sumstate);
        ops::addState(dx0, dstrdx0, dsumstate);

        // ---------------- flip key bit -----------------
        TOGGLE_BIT(key[key_word], key_bit);
        if (basic_config.key_size == 128)
            TOGGLE_BIT(key[key_word + 4], key_bit);

        // ---------------- make new X and X' with altered key bits -----------------
        salsa::insert_key(strdx0, key);
        salsa::insert_key(dstrdx0, key);

        // ---------------- Z = X - X^R -----------------
        ops::subtractState(sumstate, strdx0, minusstate);
        ops::subtractState(dsumstate, dstrdx0, dminusstate);

        // ---------------- backward round -----------------
        qr.UEVENARX_18(minusstate);
        qr.UEVENARX_18(dminusstate);

        qr.EVENARX_13(minusstate);
        qr.EVENARX_13(dminusstate);

        bckward.Half_2_EvenRF(minusstate);
        bckward.Half_2_EvenRF(dminusstate);

        if (basic_config.totalRoundsAreFractional())
        {
            if (rounded_total_rounds_are_odd)
            {
                bckward.Half_2_EvenRF(minusstate);
                bckward.Half_2_EvenRF(dminusstate);
            }
            else
            {
                bckward.Half_2_OddRF(minusstate);
                bckward.Half_2_OddRF(dminusstate);
            }
        }
        for (int i{rounded_total_rounds}; i > bwd_round; i--)
        {
            bckward.RoundFunction(minusstate, i);
            bckward.RoundFunction(dminusstate, i);
        }

        if (fwd_rounds_are_fractional)
        {
            if (rounded_fwd_rounds_are_odd)
            {
                bckward.Half_1_EvenRF(minusstate);
                bckward.Half_1_EvenRF(dminusstate);
            }
            else
            {
                bckward.Half_1_OddRF(minusstate);
                bckward.Half_1_OddRF(dminusstate);
            }
        }
        // ---------------- backward round end -----------------

        // ---------------- XOR state -----------------
        ops::xorState(minusstate, dminusstate, DiffState);

        // ---------------- store backward parity -----------------
        for (const auto &d : diff_config.mask)
            bwd_parity ^= GET_BIT(DiffState[d.first], d.second);

        // ---------------- parity check -----------------
        if (fwd_parity == bwd_parity)
            thread_match_count++;
    }

    return static_cast<double>(thread_match_count);
}

// ---------------- skip helper -----------------
inline bool skip_this(u16 idx, const vector<u16> &skip_bits)
{
    return std::binary_search(skip_bits.begin(), skip_bits.end(), idx);
}
