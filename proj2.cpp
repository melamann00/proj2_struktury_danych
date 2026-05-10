#include <iostream>
#include <vector>
#include <list>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <random>
#include <iomanip>
#include <cassert>
using namespace std;
using namespace chrono;
using Clock  = high_resolution_clock;
using Micros = microseconds;
using Nanos  = nanoseconds;

template<typename T>
class BinaryHeapPQ {
    struct Node{
        T value;
        int priority;
    };
    vector<Node> h;
    void siftUp(int i){
        for (int p; i > 0 && h[p=(i-1)/2].priority < h[i].priority; i=p){
            swap(h[i], h[p]);
        }
    }
    void siftDown(int i) {
        int n = h.size();
        for (;;) {
            int best = i, l = 2*i+1, r = 2*i+2;
            if (l < n && h[l].priority > h[best].priority){
                best = l;
            }
            if (r < n && h[r].priority > h[best].priority){
                best = r;
            }
            if (best == i){
                break;
            }
            swap(h[i], h[best]); 
            i = best;
        }
    }

public:
    void push(T v, int p){
        h.push_back({v,p});
        siftUp(h.size()-1);
    }
    void pop(){ 
        h[0]=h.back(); 
        h.pop_back(); 
        if(!h.empty()) {
            siftDown(0);
        } 
    }
    pair<T,int> peek()const { return {h[0].value, h[0].priority}; }
    int size() const{ 
        return h.size();
    }
    bool empty() const{
        return h.empty(); 
    }
    void changePriority(T v, int p) {
        for (int i=0; i<(int)h.size(); i++){
            if (h[i].value==v){ 
                int old=h[i].priority;
                h[i].priority=p; 
                p>old ? siftUp(i) : siftDown(i); 
                return; 
            }
        }
    }
};
template<typename T>
class SortedArrayPQ {
    struct Node {
        T value;
        int priority; 
    };
    vector<Node> arr;
public:
    void push(T v, int p) {
        auto it = lower_bound(arr.begin(), arr.end(), p, [](const Node& a, int b){ return a.priority < b; });
        arr.insert(it, {v,p});
    }
    void pop(){
        arr.pop_back();
    }
    pair<T,int> peek() const {
        return {arr.back().value, arr.back().priority}; 
    }
    int  size() const{ 
        return arr.size();
    }
    bool empty() const{ 
        return arr.empty();
    }

    void changePriority(T v, int p) {
        for (auto it=arr.begin(); it!=arr.end(); ++it){
            if (it->value==v) {
                arr.erase(it);
                push(v,p);
                return; 
            }
        }
    }
};

template<typename T>
class SortedListPQ {
   struct Node { T value; int priority; };
    list<Node> lst;

public:
    void push(T v, int p) {
        auto it = lst.begin();
        while (it!=lst.end() && it->priority>=p){ 
            ++it;
        };
        lst.insert(it, {v,p});
    }
    void pop(){ 
        lst.pop_front(); 
    }
    pair<T,int> peek() const { 
        return {lst.front().value, lst.front().priority}; 
    }
    int size() const{ 
        return lst.size(); 
    }
    bool empty() const{ 
        return lst.empty(); 
    }

    void changePriority(T v, int p) {
        for (auto it=lst.begin(); it!=lst.end(); ++it)
            if (it->value==v) { lst.erase(it); push(v,p); return; }
    }
};

//Test
struct Result { long long push_us, pop_us, peek_ns, size_ns, change_us; };

template<typename Fn>
long long measureNs(Fn fn, int reps=200) {
    auto t0=Clock::now();
    for(int i=0;i<reps;i++) fn();
    return duration_cast<Nanos>(Clock::now()-t0).count()/reps;
}

template<template<typename> class PQ>
Result Test(int n, const vector<int>& order) {
    Result r{};
    // push
    { 
        PQ<int> pq;
        auto t0=Clock::now();
        for(int i=0;i<n;i++) {
            pq.push(i,order[i]);
        }
        r.push_us=duration_cast<Micros>(Clock::now()-t0).count(); 
    }
    //wypełniona kolejka do pozostałych testów
    PQ<int> pq;
    for(int i=0;i<n;i++) {
        pq.push(i,order[i]);
    }
    r.peek_ns  = measureNs([&]{ volatile auto x=pq.peek();  (void)x; });
    r.size_ns  = measureNs([&]{ volatile int s=pq.size();  (void)s; });
    //changePriority
    {
        auto t0=Clock::now();
        for(int rep=0;rep<10;++rep) {
            pq.changePriority(n/2, 2'000'000+rep);
        }
        r.change_us=duration_cast<Micros>(Clock::now()-t0).count()/10; 
    }
    //pop
    { 
        PQ<int> pq2; 
        for(int i=0;i<n;i++){
            pq2.push(i,order[i]);
        }
        auto t0=Clock::now();
        while(!pq2.empty()){
            pq2.pop();
        }
        r.pop_us=duration_cast<Micros>(Clock::now()-t0).count(); 
    }
    return r;
}
//Wyswietlanie
void printHeader() {
    cout <<"Struktura   Przypadek        N        push [µs]     pop [µs]      peek [ns]     size [ns]     chgPri [µs]\n";
}
void printRow(const string& name, const string& cas, int n, const Result& r) {
    cout<<name<<setw(15)<<cas<<setw(8)<<n<<setw(14)<<r.push_us<<setw(14)<<r.pop_us<<setw(14)<<r.peek_ns<<setw(14)<<r.size_ns<<setw(16)<<r.change_us<<"\n";
}

int main() {
    //Testy jednostkowe
    mt19937 rng(124134);
    printHeader();
    for (int n : {1000, 10000, 100000}) {
        vector<int> asc(n), desc(n), rnd(n);
        iota(asc.begin(), asc.end(), 0);
        iota(desc.begin(), desc.end(), 0); reverse(desc.begin(), desc.end());
        rnd = asc; shuffle(rnd.begin(), rnd.end(), rng);
        printRow("BinaryHeap", "optimistyczny",n, Test<BinaryHeapPQ>(n, asc));
        printRow("BinaryHeap", "sredni",n, Test<BinaryHeapPQ>(n, rnd));
        printRow("BinaryHeap", "pesymistyczny",n, Test<BinaryHeapPQ>(n, desc));
        printRow("SortedArray", "optimistyczny",n, Test<SortedArrayPQ>(n, asc));
        printRow("SortedArray", "sredni",n, Test<SortedArrayPQ>(n, rnd));
        printRow("SortedArray", "pesymistyczny",n, Test<SortedArrayPQ>(n, desc));
        printRow("SortedList", "optimistyczny",n, Test<SortedListPQ>(n, desc));
        printRow("SortedList", "sredni",n, Test<SortedListPQ>(n, rnd));
        printRow("SortedList", "pesymistyczny",n, Test<SortedListPQ>(n, asc));
        cout<<"\n";
    }

    cout << R"(
Operacja          BinaryHeap      SortedArray     SortedList
push              O(log n)        O(n)            O(n)
pop               O(log n)        O(1)            O(1)
peek              O(1)            O(1)            O(1)
size              O(1)            O(1)            O(1)
changePriority    O(n + log n)    O(n)            O(n)
)";
}
