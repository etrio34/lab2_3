// Variant 12: scheme #12, nt=4
// a=4, b=8, c=6, d=9, e=5, f=5, g=9, h=8, i=7, j=7, k=2, m=4, n=3
//
// Scheme (node -> incoming sets / outgoing sets):
//   M1: in {a, c}  -> out {b}
//   M2: in {g, h}  -> out {i, j}
//   M3: in {d, e}  -> out {f}
//   N1: in {b, i}  -> out {m}
//   N2: in {j}     -> out {k}
//   N3: in {f}     -> out {n}
//   END: in {m, k, n}

#include <chrono>
#include <iostream>
#include <latch>
#include <syncstream>
#include <thread>

using namespace std::chrono_literals;

// One latch per set of actions; the counter = number of actions in the set.
static std::latch done_a(4), done_b(8), done_c(6), done_d(9), done_e(5),
                  done_f(5), done_g(9), done_h(8), done_i(7), done_j(7),
                  done_k(2), done_m(4), done_n(3);

// Stub of an action: sleeps for 1 second and prints a message.
void f(char set, int i) {
    std::this_thread::sleep_for(1s);
    std::osyncstream(std::cout) << "Action " << i << " of set " << set << " done.\n";
}

// Hardcoded dependencies: wait until all actions of the parent sets are done.
static void wait_parents(char set) {
    switch (set) {
        case 'b': done_a.wait(); done_c.wait(); break;
        case 'i':
        case 'j': done_g.wait(); done_h.wait(); break;
        case 'f': done_d.wait(); done_e.wait(); break;
        case 'm': done_b.wait(); done_i.wait(); break;
        case 'k': done_j.wait(); break;
        case 'n': done_f.wait(); break;
        default: break;  // a, c, d, e, g, h have no incoming dependencies
    }
}

// Signals that one action of the set is finished.
static void signal_done(char set) {
    switch (set) {
        case 'a': done_a.count_down(); break;
        case 'b': done_b.count_down(); break;
        case 'c': done_c.count_down(); break;
        case 'd': done_d.count_down(); break;
        case 'e': done_e.count_down(); break;
        case 'f': done_f.count_down(); break;
        case 'g': done_g.count_down(); break;
        case 'h': done_h.count_down(); break;
        case 'i': done_i.count_down(); break;
        case 'j': done_j.count_down(); break;
        case 'k': done_k.count_down(); break;
        case 'm': done_m.count_down(); break;
        case 'n': done_n.count_down(); break;
    }
}

// Runs the i-th action of the set: wait for dependencies, execute, signal.
static void act(char set, int i) {
    wait_parents(set);
    f(set, i);
    signal_done(set);
}

// ---- Hardcoded plan: 20 time units, 77 actions, 4 threads ----

void worker1() {
    act('g', 1); act('g', 5); act('g', 9); act('h', 4); act('h', 8);   // t=1..5
    act('i', 1); act('i', 5); act('j', 2); act('j', 6);                // t=6..9
    act('a', 3); act('c', 3);                                          // t=10..11
    act('d', 4); act('d', 8); act('e', 3);                             // t=12..14
    act('f', 1); act('f', 5);                                          // t=15..16
    act('b', 5); act('m', 1); act('k', 1); act('n', 3);                // t=17..20
}

void worker2() {
    act('g', 2); act('g', 6); act('h', 1); act('h', 5); act('d', 1);   // t=1..5
    act('i', 2); act('i', 6); act('j', 3); act('j', 7);                // t=6..9
    act('a', 4); act('c', 4);                                          // t=10..11
    act('d', 5); act('d', 9); act('e', 4);                             // t=12..14
    act('f', 2); act('b', 2); act('b', 6);                             // t=15..17
    act('m', 2); act('k', 2);                                          // t=18..19
}

void worker3() {
    act('g', 3); act('g', 7); act('h', 2); act('h', 6); act('d', 2);   // t=1..5
    act('i', 3); act('i', 7); act('j', 4); act('a', 1);                // t=6..9
    act('c', 1); act('c', 5);                                          // t=10..11
    act('d', 6); act('e', 1); act('e', 5);                             // t=12..14
    act('f', 3); act('b', 3); act('b', 7);                             // t=15..17
    act('m', 3); act('n', 1);                                          // t=18..19
}

void worker4() {
    act('g', 4); act('g', 8); act('h', 3); act('h', 7); act('d', 3);   // t=1..5
    act('i', 4); act('j', 1); act('j', 5); act('a', 2);                // t=6..9
    act('c', 2); act('c', 6);                                          // t=10..11
    act('d', 7); act('e', 2); act('b', 1);                             // t=12..14
    act('f', 4); act('b', 4); act('b', 8);                             // t=15..17
    act('m', 4); act('n', 2);                                          // t=18..19
}

int main() {
    const auto start = std::chrono::steady_clock::now();
    std::osyncstream(std::cout) << "Computation started.\n";

    {
        // main only creates the threads and waits for them (jthread joins
        // in its destructor); it performs no actions itself.
        std::jthread t1(worker1), t2(worker2), t3(worker3), t4(worker4);
    }

    const std::chrono::duration<double> elapsed =
        std::chrono::steady_clock::now() - start;
    std::osyncstream(std::cout) << "Computation finished. Duration: "
                                << elapsed.count() << " seconds.\n";
    return 0;
}
