#define _CRT_SECURE_NO_DEPRECATE
#define _HAS_ITERATOR_DEBUGGING 0
#include <iostream>
#include <stdio.h>
#include <fstream>
#include <vector>
#include <math.h>
#include <algorithm>
#include <iterator>
#include <stdlib.h>
#include <time.h>
using namespace std;

#define PI 3.14159265358979
#define POPSIZE 75				// population size
#define MAXGENS 100000			// Max number of generations
#define PXOVER 0.73				// Probability of crossover
#define PMUTATION 0.76			// Probability of mutation

int numberOfPoints;
double currentKnownBest, capacity;

int generation;					// Current generation number
double dist[400][400];			// dist[i][j] saves the distance from i to j
int countGen;					// Count the generations that fitness change within 0.01
double fitnessUpBound = 0;		//Record the fitness's value's upbound
double fitnessLowBound = LLONG_MAX;		// Record the fitness's value's lowbound

/* Information of each node */
struct node
{
	int x, y;					// Coordinate of the point
	int No;						// No. of the point
	double angleValue;			// Angle value of the point
	double demand;				// Demand of the node
};

vector<node> cityByOrder;		// Stores data of each node by order
vector<node> cities;			// Stores data of each node, cities[0] is the depot, cities[1]~cities[numberOfPoints] are customers

struct genotype
{
	vector<vector<int> > vehicleServices;	// Store the points each vehicle serves
	int pointSequence[POPSIZE];	// Point sequence
	double fitness;				// Fitness
	double rfitness;			// Relative fitness
	double cfitness;			// cumulative fitness
};

genotype population[POPSIZE];	// Population
genotype newPopulation[POPSIZE];	// New population that might replace the old population
genotype currentBest;			// Store the information of the best gene
genotype chromosomeA, chromosomeB;

/* Calculate the distance between i and j and save in dist[i][j] */
void calDist()
{
	for (int i = 0; i <= numberOfPoints; i++)
	{
		for (int j = 0; j <= numberOfPoints; j++)
		{
			dist[i][j] = sqrt(pow(cities[j].x - cities[i].x, 2) + pow(cities[j].y - cities[i].y, 2));
			dist[j][i] = dist[i][j];
		}
	}
}

/* Calculate the angle value of each city */
void calAngleValue()
{
	for (int i = 1; i <= numberOfPoints; i++)
	{
		if (cities[i].x > 0)
		{
			if (cities[i].y >= 0)
				cities[i].angleValue = atan(double(cities[i].y / cities[i].x));
			else
				cities[i].angleValue = 2 * PI + atan(double(cities[i].y / cities[i].x));
		}
		else if (cities[i].x == 0)
		{
			if (cities[i].y > 0)
				cities[i].angleValue = double(PI / 2);
			else
				cities[i].angleValue = double(PI / 2) + PI;
		}
		else
			cities[i].angleValue = PI + atan(double(cities[i].y / cities[i].x));
	}
}

/* Helper function to sort the vector cities by their angle value */
bool cmpAngleValue(node a, node b)
{
	return a.angleValue < b.angleValue;
}

/* Sort the points according to their angle value. The smallest is in the front */
void sortPoints()
{
	sort(cities.begin() + 1, cities.end(), cmpAngleValue);
}

/* Initialize the sequence each vehicle serves */
void initializeServeSequence()
{
	double totalDemands = 0;
	for (int i = 0; i < POPSIZE; i++)
	{
		int k = 0;
		for (int j = i + 1; j < cities.size(); j++)
		{
			if (totalDemands + cities[j].demand < capacity)
			{
				totalDemands += cities[j].demand;
				population[i].pointSequence[k] = cities[j].No;
				k++;
			}
			else
			{
				totalDemands = 0;
				j--;
			}
		}
		for (int j = 1; j <= i; j++)
		{
			if (totalDemands + cities[j].demand < capacity)
			{
				totalDemands += cities[j].demand;
				population[i].pointSequence[k] = cities[j].No;
				k++;
			}
			else
			{
				totalDemands = 0;
				j--;
			}
		}
	}
}

/* Decide the points each vehicle serves */
void decideServePoints()
{
	for (int j = 0; j < POPSIZE; j++)
	{
		vector<int> vehicle;
		double totalDemands = 0;
		population[j].vehicleServices.clear();
		for (int i = 0; i < POPSIZE; i++)
		{
			if (totalDemands + cityByOrder[population[j].pointSequence[i]].demand < capacity)
			{
				totalDemands += cityByOrder[population[j].pointSequence[i]].demand;
				vehicle.push_back(population[j].pointSequence[i]);
			}
			else
			{
				population[j].vehicleServices.push_back(vehicle);
				vehicle.clear();
				totalDemands = 0;
				i--;
			}
		}
		if (!vehicle.empty())
			population[j].vehicleServices.push_back(vehicle);
		population[j].fitness = 0;
	}
}

/* Decide the route of each vehicle based on the points they serves */
void decideVehicleRoute()
{
	for (int m = 0; m < POPSIZE; m++)
	{
		int k = 0;
		for (int i = 0; i < population[m].vehicleServices.size(); i++)
		{
			vector<int> tmp;
			copy(population[m].vehicleServices[i].begin(), population[m].vehicleServices[i].end(), back_inserter(tmp));
			population[m].vehicleServices[i].clear();
			int now = 0, size = tmp.size();
			for (int j = 0; j < size; j++)
			{
				double minDist = INT_MAX;
				int mark = 0;
				for (int k = 0; k < tmp.size(); k++)
				{
					if (dist[now][tmp[k]] < minDist)
					{
						minDist = dist[now][tmp[k]];
						mark = k;
					}
				}
				population[m].vehicleServices[i].push_back(tmp[mark]);
				// Update population[i].pointSequence according to the routes decided above
				population[m].pointSequence[k++] = tmp[mark];
				now = tmp[mark];
				tmp.erase(tmp.begin() + mark);
			}
		}
	}
}

/* Initialize each gene and its fitness */
void initialize()
{
	calDist();
	calAngleValue();
	sortPoints();
	initializeServeSequence();
	decideServePoints();
	decideVehicleRoute();
	currentBest.fitness = LLONG_MAX;
}

/* Compute each gene's fitness */
void evaluate()
{
	for (int i = 0; i < POPSIZE; i++)
	{
		double geneFitness = 0;
		for (int j = 0; j < population[i].vehicleServices.size(); j++)
		{
			int now = 0;
			for (int k = 0; k < population[i].vehicleServices[j].size(); k++)
			{
				geneFitness += dist[now][population[i].vehicleServices[j][k]];
				now = population[i].vehicleServices[j][k];
			}
			geneFitness += dist[now][0];
		}
		population[i].fitness = geneFitness;
	}
}

/* Keep track of the best member of the population */
void keepTheBest()
{
	bool flag = true;
	for (int i = 0; i < POPSIZE; i++)
	{
		if (population[i].fitness < currentBest.fitness)
		{
			currentBest = population[i];
			countGen = 0;
			flag = false;
		}
	}
	if (flag)
		countGen++;
	
	cout << currentBest.fitness << endl;
}

/* Decide when to end the while loop */
bool stopRule()
{
	if (generation == 100000 || countGen == 10000)
		return true;
	return false;
}

/* Use Roulette Wheel Selection to generate the new population */
void select()
{
	double sum = 0, randomNum;
	// Find total fitness of the population
	for (int i = 0; i < POPSIZE; i++)
		sum += 10000 / population[i].fitness;

	// Calculate relative fitness
	for (int i = 0; i < POPSIZE; i++)
		population[i].rfitness = (10000 / population[i].fitness) / sum;

	// Calculate cumulative fitness
	population[0].cfitness = population[0].rfitness;
	for (int i = 1; i < POPSIZE; i++)
		population[i].cfitness = population[i - 1].cfitness + population[i].rfitness;

	// Select survivors using cumulative fitness
	srand(time(NULL));
	for (int i = 0; i < POPSIZE; i++)
	{
		randomNum = rand() % 1000 / 1000.0;
		if (randomNum < population[0].cfitness)
		{
			newPopulation[i] = population[0];
		}
		else
		{
			for (int j = 0; j < POPSIZE; j++)
			{
				if (randomNum >= population[j].cfitness && randomNum < population[j + 1].cfitness)
				{
					newPopulation[i] = population[j + 1];
					break;
				}
			}
		}
	}

	// Copy the new population back to the old population
	for (int i = 0; i < POPSIZE; i++)
		population[i] = newPopulation[i];
}

/* Xover's helper function to decide whether num is in tmp */
bool findNum(vector<int> tmp, int num)
{
	for (int i = 0; i < tmp.size(); i++)
	{
		if (tmp[i] == num)
			return true;
	}
	return false;
}

/* Crossover's helper function process the corssover using Order Crossover */
void Xover(int one, int two)
{
	int oneChild[POPSIZE], twoChild[POPSIZE];
	int index1, index2, j;
	index1 = index2 = 0;
	vector<int> tmp1, tmp2;
	srand(time(NULL));
 	while (index1 == index2)
	{
		index1 = rand() % POPSIZE;
		index2 = rand() % POPSIZE;
	}
	if (index1 > index2)
	{
		int tmp = index1;
		index1 = index2;
		index2 = tmp;
	}

	// Copy from parents to children the sequence exactly from index1 to index2
	for (int i = index1; i <= index2; i++)
	{
		oneChild[i] = population[one].pointSequence[i];
		twoChild[i] = population[two].pointSequence[i];
		tmp1.push_back(population[one].pointSequence[i]);
		tmp2.push_back(population[two].pointSequence[i]);
	}

	// Fill the left blanks in oneChild using population[two]
	j = 0;
	for (int i = 0; i < POPSIZE; i++)
	{
		if (i == index1)
		{
			if (index2 + 1 == POPSIZE)
				break;
			else
				i = index2 + 1;
		}
		while (findNum(tmp1, population[two].pointSequence[j]))
		{
			j++;
		}
		oneChild[i] = population[two].pointSequence[j];
		j++;
	}

	// Fill the left blanks in twoChild using population[one]
	j = 0;
	for (int i = 0; i < POPSIZE; i++)
	{
		if (i == index1)
		{
			if (index2 + 1 == POPSIZE)
				break;
			else
				i = index2 + 1;
		}
		while (findNum(tmp2, population[one].pointSequence[j]))
		{
			j++;
		}
		twoChild[i] = population[one].pointSequence[j];
		j++;
	}

	// Change population[one] and population[two] to oneChild and twoChild
	for (int i = 0; i < POPSIZE; i++)
	{
		population[one].pointSequence[i] = oneChild[i];
		population[two].pointSequence[i] = twoChild[i];
	}
}

/* Select two parents that take part in the corssover */
void crossover()
{
	int chosen = 0, one;				// Count the number of members chosen
	for (int i = 0; i < POPSIZE / 2; i++)
	{
		srand(time(NULL));
		double x = rand() % 1000 / 1000.0;
		if (x < PXOVER)
		{
			/*chosen++;
			if (chosen % 2 == 0)
				Xover(one, i);
			else
				one = i;*/
			Xover(i, POPSIZE - i - 1);
		}
	}
}

/* Exchange random two genes in the population selected for mutation */
void mutate()
{
	for (int i = 0; i < POPSIZE; i++)
	{
		srand(time(NULL));
		double x = rand() % 1000 / 1000.0;
		if (x < PMUTATION)
		{
			int index1, index2;
			index1 = index2 = 0;
			while (index1 == index2)
			{
				index1 = rand() % POPSIZE;
				index2 = rand() % POPSIZE;
			}
			int tmp = population[i].pointSequence[index1];
			population[i].pointSequence[index1] = population[i].pointSequence[index2];
			population[i].pointSequence[index2] = tmp;
		}
	}
}

/* Find the best population after crossover and store it in chromosomeB */
void findTheBest()
{
	decideServePoints();
	decideVehicleRoute();
	evaluate();
	chromosomeB.fitness = LLONG_MAX;
	for (int i = 0; i < POPSIZE; i++)
	{
		if (population[i].fitness < chromosomeB.fitness)
		{
			chromosomeB = population[i];
		}
	}
}

/* Find two worst population after mutation and replace them with chromosomeA and chromosomeB */
void findTheWorst()
{
	decideServePoints();
	decideVehicleRoute();
	evaluate();
	int index1, index2, worst1, worst2;
	index1 = index2 = worst1 = worst2 = 0;
	for (int i = 0; i < POPSIZE; i++)
	{
		if (population[i].fitness > worst1)
		{
			worst1 = population[i].fitness;
			index1 = i;
		}
	}
	for (int i = 0; i < POPSIZE; i++)
	{
		if (population[i].fitness > worst2 && population[i].fitness <= worst1)
		{
			worst2 = population[i].fitness;
			index2 = i;
		}
	}
	population[index1] = chromosomeA;
	population[index2] = chromosomeB;
}

int main()
{
	fstream inout;
	inout.open("tai75a.dat", ios::in);
	inout >> numberOfPoints >> currentKnownBest >> capacity;

	node depot;
	inout >> depot.x >> depot.y;
	depot.demand = depot.angleValue = depot.No = 0;
	cities.push_back(depot);
	cityByOrder.push_back(depot);

	for (int i = 0; i < numberOfPoints; i++)
	{
		node city;
		int num;
		inout >> num >> city.x >> city.y >> city.demand;
		city.angleValue = 0;
		city.No = i + 1;
		cities.push_back(city);
		cityByOrder.push_back(city);
	}
	inout.close();
	inout.open("res.txt", ios::out);

	generation = 0;
	initialize();
	evaluate();
	keepTheBest();
	while (!stopRule())
	{
		generation++;
		chromosomeA = currentBest;
		select();
		crossover();
		findTheBest();
		mutate();
		findTheWorst();
		currentBest = chromosomeA;
		decideServePoints();
		decideVehicleRoute();
		evaluate();
		keepTheBest();

		inout << generation << "\t" << currentBest.fitness << endl;
	}
	inout.close();

	cout << endl << generation << endl;

	system("pause");
	return 0;
}