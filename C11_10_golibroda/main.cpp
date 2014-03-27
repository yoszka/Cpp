#define __cplusplus 201103L
#include <iostream>
#include <mutex>
#include <array>
#include <vector>
#include <thread>
#include <random>
#include <functional>
#include <string>
#include <condition_variable>


using namespace std;
// ************************************************************
mutex mtx_golibrody;
mutex mtx_cout;
mutex mtx_krzesla;
mutex mtx_poczekalnia;
condition_variable  cv_golibrodaWaitLock;
condition_variable  cv_klientWaitLock;
const int           iloscMiejscWPoczekalni      = 5;
int                 iloscKlientowWPoczekalni    = 0;
int                 iloscKlientowDoObsluzenia   = 50;
int                 iloscRoznychKlientow        = 9;

// ************************************************************
/**
 * Generator liczb losowych z przedzialu
 */
int rand2(int from, int to) {
    // Seed with a real random value, if available
    random_device rd;

    default_random_engine engine(rd());
    uniform_int_distribution<int> distribution(from, to);
    return distribution(engine);
}
// ************************************************************
/**
 * Timeout dla podanego losowego przedzialu
 */
chrono::milliseconds to() {
    chrono::milliseconds timeout(rand2(1, 20));
    return timeout;
}
// ************************************************************
void golibroda_thread_func() {
    while(true) {
        unique_lock<mutex> lk(mtx_golibrody);
        cv_golibrodaWaitLock.wait(lk, []{return (iloscKlientowWPoczekalni > 0);});	// golibroda warunkowo zasypia (budzi sie gdy w poczekalni pojawia sie klienci)

        mtx_poczekalnia.lock();
        --iloscKlientowWPoczekalni;
        --iloscKlientowDoObsluzenia;
        mtx_poczekalnia.unlock();

        mtx_cout.lock();
        cout << "#Golibroda pracuje, w poczekalni: " << iloscKlientowWPoczekalni << ", pozostalo klientow: " << iloscKlientowDoObsluzenia << endl;
        mtx_cout.unlock();

        this_thread::sleep_for(to());   								// random sleep - golibroda wykonuje prace

        if(iloscKlientowDoObsluzenia == 0) {							// warunek zakonczenia watku
            mtx_cout.lock();
            cout << "HURRA Golibroda moze juz isc na emeryture" << endl;
            mtx_cout.unlock();
            cv_klientWaitLock.notify_all();								// powiadom wszystkich klentow
            return;
        }
        cv_klientWaitLock.notify_one();									// zawolaj kolejnego klienta

    }
}
// ************************************************************
void klient_thread_func(int numer) {
	bool jestWPoczekalniLubObslugiwany = false;

    while(true) {
        bool mozewejscDoPoczekalni = false;

        mtx_poczekalnia.lock();
            if(!jestWPoczekalniLubObslugiwany
               &&
               iloscKlientowWPoczekalni < iloscMiejscWPoczekalni
               &&
               iloscKlientowWPoczekalni < iloscKlientowDoObsluzenia) {

                iloscKlientowWPoczekalni++;
                mozewejscDoPoczekalni = true;
            }
        mtx_poczekalnia.unlock();

        if(mozewejscDoPoczekalni) {
			jestWPoczekalniLubObslugiwany = true;

            mtx_cout.lock();
			cout << "Klient "<< numer <<" w poczekalni, bedzie obslugiwany, oprocz niego jest innych kliento: " << (iloscKlientowWPoczekalni - 1) << endl;
            mtx_cout.unlock();

            this_thread::sleep_for(to());   		// random sleep - czas klienta na rozgoszczenie sie w poczekalni
            cv_golibrodaWaitLock.notify_one();		// jesliby golibroda spal to daj mu znac ze przyszedl klient

        } else {
            unique_lock<mutex> lk(mtx_krzesla);								// skoro nie udalo sie wejsc do poczekalni to bedziemy czekac gdzies indziej az bedzie mozna wejsc
            cv_klientWaitLock.wait(lk);

            jestWPoczekalniLubObslugiwany = false;
        }

        if(iloscKlientowWPoczekalni == iloscKlientowDoObsluzenia) {			// warunek zakonczenia watku - w poczekalni jest tyle kleinow, co wystarczy golibrodzie zeby ich obsluzyl i poszedl na emeryture
            mtx_cout.lock();
            cout << "Klient " << numer << " zmienil golibrode, pozostalo " << --iloscRoznychKlientow << " klentow" << endl;
            mtx_cout.unlock();
            return;
        }

    }
}
// ************************************************************

int main()
{
    cout << "START\n";

    thread golibroda(golibroda_thread_func);

    vector<thread> klienci;
    klienci.push_back(thread(klient_thread_func, 1));
    klienci.push_back(thread(klient_thread_func, 2));
    klienci.push_back(thread(klient_thread_func, 3));
    klienci.push_back(thread(klient_thread_func, 4));
    klienci.push_back(thread(klient_thread_func, 5));
    klienci.push_back(thread(klient_thread_func, 6));
    klienci.push_back(thread(klient_thread_func, 7));
    klienci.push_back(thread(klient_thread_func, 8));
    klienci.push_back(thread(klient_thread_func, 9));

    for(thread& th : klienci) {
        th.join();
    }

    golibroda.join();
}
