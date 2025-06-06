// System Headers
#include <iostream>
#include <cmath>
#include <chrono>
#include <thread>
#include <vector>
#include <mutex>
#include <cmath>



// Project Headers
#include "nbody.h"

// #define GRAPHICS
#ifdef GRAPHICS
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#endif

//HAd issues with the math library and the M_PI constant
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Number of particles
 #define SMALL
// #define LARGE
// #define MASSIVE

// Define TESTING
const bool TEST = false;

//Define original function for comparison
const bool ORIGINAL = false;

#if defined(SMALL)
const int N = 1000;
#elif defined(LARGE)
const int N = 5000;
#elif defined(MASSIVE)
const int N = 10000;
#endif

//----------------------
using namespace std;
//----------------------
// Constants
const double min2 = 2.0;
const double G = 1 * 10e-10;
const double dt = 0.01;
const int NO_STEPS = 500;

// Size of Window/Output image
const int width = 1920;
const int height = 1080;

// Bodies
// body bodies[N];
body *bodies = new body[N];

// Set the number of threads
const int NUM_THREADS = thread::hardware_concurrency() > 0 ? thread::hardware_concurrency() : 1; // Use hardware concurrency if available, otherwise default to 4 threads

// Mutex for thread safety
mutex cout_mtx;

// Count for testing and limiting cout
int TEST_COUNT = 0;


// Print function
void printID()
{
	cout << "----------------------------------------" << endl;
	cout << "  159.341 Assignment 3 Semester 1 2025  " << endl;
	cout << "   Submitted by: Zak Turner, 23003797   " << endl;
	cout << "----------------------------------------" << endl;
}

// helper functions

void thread_forces(int t, int start, int end, vec2 *l_acc)
{
	if (TEST && TEST_COUNT % 100 == 0)
	{
		lock_guard<mutex> lock(cout_mtx);
		cout << "Thread " << t << " calculating forces for bodies " << start << " to " << end - 1 << " " << endl;
	}

	 // Initialize the local acceleration array to all zeros
    // for (int i = 0; i < N; ++i)
    // {
    //     l_acc[i] = vec2(0, 0);
    // }

	// For each body in our assigned range //Used from original UPDATE function
	for (int i = start; i < end; ++i)
	{
		

		// For each following body - calculate interaction
		for (int j = i + 1; j < N; ++j)
		{
			// Difference in position
			vec2 dx = bodies[i].pos - bodies[j].pos;

			// Normalised difference in position
			vec2 u = normalise(dx);

			// Calculate distance squared
			double d2 = length2(dx);

			// If greater than minimum distance
			if (d2 > min2)
			{
				// Smoothing factor for particles close to minimum distance
				double x = smoothstep(min2, 2 * min2, d2);

				// Force between bodies
				double f = -G * bodies[i].mass * bodies[j].mass / d2;

				// Add to acceleration
				l_acc[i] += (u * f / bodies[i].mass) * x;
				l_acc[j] -= (u * f / bodies[j].mass) * x;
			}
		}
	}
}
// Update Nbody Simulation
void update_original()
{
	// Acceleration
	vec2 acc[N];

	// Clear Acceleration
	for (int i = 0; i < N; ++i)
	{
		acc[i] = vec2(0, 0);
	}

	// For each body
	for (int i = 0; i < N; ++i)
	{
		// For each following body
		for (int j = i + 1; j < N; ++j)
		{
			// Difference in position
			vec2 dx = bodies[i].pos - bodies[j].pos;

			// Normalised difference in position
			vec2 u = normalise(dx);

			// Calculate distance squared
			double d2 = length2(dx);

			// If greater than minimum distance
			if (d2 > min2)
			{
				// Smoothing factor for particles close to minimum distance
				double x = smoothstep(min2, 2 * min2, d2);

				// Force between bodies
				double f = -G * bodies[i].mass * bodies[j].mass / d2;

				// Add to acceleration
				acc[i] += (u * f / bodies[i].mass) * x;
				acc[j] -= (u * f / bodies[j].mass) * x;
			}
		}
	}

	// For each body
	for (int i = 0; i < N; ++i)
	{
		// Update Position
		bodies[i].pos += bodies[i].vel * dt;

		// Update Velocity
		bodies[i].vel += acc[i] * dt;
	}
}

void update()
{
	
	// // Acceleration
	// 	vec2 acc[N];

	 // Use a global acceleration array and allocate on the heap instead of the stack (issues with stack overflow see notes) seperating concerns from the forces array
    vec2 *acc = new vec2[N];

	// Clear Acceleration
	for (int i = 0; i < N; ++i)
	{
		acc[i] = vec2(0, 0);
	}

	// Distibution between the threads
	int bodies_distributed = N / NUM_THREADS; // Total number of bodies DIV by the number of threads to get the number of bodies per thread (Tried float division but was problematic in arrays! And just problematic)
	int bodies_left = N % NUM_THREADS;		 // The remainder, if any

	if (TEST && TEST_COUNT % 100 == 0 ) //Limit output
	{
		lock_guard<mutex> lock(cout_mtx);
		cout << "Distibuting bodies between threads: " << N << "/" << NUM_THREADS << " = " << bodies_distributed << endl;
		cout << "Leftover bodies: " << bodies_left << endl;
		cout << "Total = " << "(Bodies distributed * Number of threads) + Remainder = (" << bodies_distributed << " * " << NUM_THREADS << ") + " << bodies_left << " = " << (bodies_distributed * NUM_THREADS) + bodies_left << endl;
	}

 // Create thread-local acceleration array, was receiving race conditions when using a single global array
    vector<vec2*> t_l_acc;
	
    for (int t = 0; t < NUM_THREADS; ++t)
    {
        vec2 *l_acc = new vec2[N];
        // Initialize thread-local array to zero
        for (int i = 0; i < N; ++i)
        {
            l_acc[i] = vec2(0, 0);
        }
        t_l_acc.push_back(l_acc);
    }


	// Vector to hold threads
	std::vector<std::thread> threads;

	for (int t = 0; t < NUM_THREADS; ++t)
	{ // calculate this threads range
		int start = t * bodies_distributed;
		int end;
	
	// Last thread gets the remaining bodies
	if (t == NUM_THREADS - 1) 
	{
		end = start + bodies_distributed + bodies_left;
	}
	else //No remainder
	{
		end = start + bodies_distributed;
	}

	 if (TEST && TEST_COUNT % 100 == 0) 
	 {
			lock_guard<mutex> lock(cout_mtx);
            cout << "Thread " << t << " assigned bodies " << start << " to " << (end-1) << " (" << (end-start) << " bodies) " << endl;
        }


		threads.push_back(thread(thread_forces, t, start, end, t_l_acc[t])); //Adding the thread to the vector, passing the thread number, start and end indices, and the local acceleration array for that thread

	}

	// Waiting
	for (auto &thread : threads)
	{
		thread.join();
	}

	// if (TEST)
	// {
	// 	cout << "Threads completed" << endl;
	// }
	
	    // Combine all thread-local arrays into global array
    for (int t = 0; t < NUM_THREADS; ++t)
    {
        for (int i = 0; i < N; ++i)
        {
            acc[i] += t_l_acc[t][i];
        }
    }


	// For each body
	for(int i = 0; i < N; ++i) {
		
		// Update Position
		bodies[i].pos += bodies[i].vel * dt;

		// Update Velocity
		bodies[i].vel += acc[i] * dt;
	}

	//Clean up 
    for (int t = 0; t < NUM_THREADS; ++t)
    {
        delete[] t_l_acc[t];
    }
    delete[] acc;

		TEST_COUNT++;
	// 	cout << TEST_COUNT << endl;

	if (TEST && TEST_COUNT == NO_STEPS )
	{
		cout << "Implementing update" << endl;
		cout << "----------------------------------------" << endl;
		 cout << "Distributing " << N << " bodies across " << NUM_THREADS << " threads" << endl;

		 cout << "----------------------------------------" << endl;
	}

	// 	if (TEST && TEST_COUNT == NO_STEPS) // Only print x1
	// {
	// 	cout << "----------------------------------------" << endl;
	// 	cout << "Testing with " << NUM_THREADS << " threads" << endl;
	// 	cout << "Supported thread count: " << thread::hardware_concurrency() << endl;
	// 	cout << "----------------------------------------" << endl;
	// }

	
	if (TEST && TEST_COUNT == NO_STEPS) {
		cout << "Parallel update complete" << endl;
	}

}
// Initialise NBody Simulation
void initialise()
{
	// Create a central heavy body (sun)
	bodies[0] = body(width / 2, height / 2, 0, 0, 1e15, 5);

	// For each other body
	for (int i = 1; i < N; ++i)
	{
		// Pick a random radius, angle and calculate velocity
		double r = (uniform() + 0.1) * height / 2;
		double theta = uniform() * 2 * M_PI;
		double v = sqrt(G * (bodies[0].mass + bodies[i].mass) / r);

		// Create orbiting body
		bodies[i] = body(width / 2 + r * cos(theta), height / 2 + r * sin(theta), -sin(theta) * v, cos(theta) * v, 1e9, 2);
	}
}

#ifdef GRAPHICS
// Main Function - Graphical Display
int main()
{
	// Create Window
	sf::ContextSettings settings;
	settings.AntialiasingLevel = 1;
	sf::RenderWindow window(sf::VideoMode(width, height), "NBody Simulator", sf::Style::Default, settings);

	// Initialise NBody Simulation
	initialise();
	int i = 0;
	// run the program as long as the window is open
	while (window.isOpen())
	{
		// check all the window's events that were triggered since the last iteration of the loop
		sf::Event event;
		while (window.pollEvent(event))
		{
			// "close requested" event: we close the window
			if (event.type == sf::Event::Closed)
			{
				window.close();
			}
		}

		if (i < NO_STEPS)
		{
			// Update NBody Simluation
			update();
			i++;
		}

		// Clear the window with black color
		window.clear(sf::Color::Black);

		// Render Objects
		for (int i = 0; i < N; ++i)
		{
			// Create Circle
			sf::CircleShape shape(bodies[i].radius);
			shape.setFillColor(sf::Color(255, 0, 0));
			shape.setPosition(bodies[i].pos.x, bodies[i].pos.y);

			// Draw Object
			window.draw(shape);
		}

		// Display Window
		window.display();
	}
}
#else
// Main Function - Benchmark
int main()
{

	// Print ID
	printID();

	// Initialise NBody Simulation
	initialise();


	// Get start time
	std::chrono::system_clock::time_point start = std::chrono::system_clock::now();

	// Run Simulation
	if (ORIGINAL)
	{
		cout << "RUNNING ORIGINAL UPDATE!" << endl;
	}
	else
	{
		cout << "RUNNING PARALLEL UPDATE!" << endl;
		cout << "----------------------------------------" << endl;
		cout << "Testing with " << NUM_THREADS << " threads" << endl;
		cout << "Supported thread count: " << thread::hardware_concurrency() << endl;
		cout << "----------------------------------------" << endl;
	}

	for (int i = 0; i < NO_STEPS; i++)
	{
		// Update NBody Simluation
		if(ORIGINAL)
		{			
			update_original();
		}
		else
		{
			update();
		}
		
	}

	// Get end time
	std::chrono::system_clock::time_point end = std::chrono::system_clock::now();

	// Generate output image
	unsigned char *image = new unsigned char[width * height * 3];
	memset(image, 0, width * height * 3);

	// For each body
	for (int i = 0; i < N; ++i)
	{
		// Get Position
		vec2 p = bodies[i].pos;

		// Check particle is within bounds
		if (p.x >= 0 && p.x < width && p.y >= 0 && p.y < height)
		{
			// Add a red dot at body
			image[((((int)p.y * width) + (int)p.x) * 3)] = 255;
		}
	}

	// Write position data to file
	char data_file[200];
	sprintf(data_file, "output%i.dat", N);
	write_data(data_file, bodies, N);

	// Write image to file
	char image_file[200];
	sprintf(image_file, "output%i.png", N);
	write_image(image_file, bodies, N, width, height);

	// Check Results
	char reference_file[200];
	sprintf(reference_file, "reference%i.dat", N);
	calculate_maximum_difference(reference_file, bodies, N);

	// Time Taken
	std::cout << "Time Taken: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000000.0 << std::endl;

return 0;
}
#endif