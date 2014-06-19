#define _CRT_SECURE_NO_DEPRECATE
#include <iostream>
#include <stdio.h>
#include <string>
#include <fstream>
#include <math.h>
#include <map>
using namespace std;

#define SIZE 150					// Point size
#define K 10						// Define K's value

map<double, int> dist;				// Stores the distance from the testing point to the training points

struct Iris
{
	double sepalLength, sepalWidth, petalLength, petalWidth;
	string classBelong;
};

Iris training[SIZE];
Iris testing[SIZE];
Iris selected[K];

/* Compute the distance between training points and testing[point] */
void computeDist(int point)
{
	dist.clear();
	for (int i = 0; i < SIZE; i++)
	{
		double distanceTmp = 0;
		distanceTmp += pow((training[i].sepalLength - testing[point].sepalLength), 2);
		distanceTmp += pow((training[i].sepalWidth - testing[point].sepalWidth), 2);
		distanceTmp += pow((training[i].petalLength - testing[point].petalLength), 2);
		distanceTmp += pow((training[i].petalWidth - testing[point].petalWidth), 2);
		dist.insert(pair<double, int>(distanceTmp, i));
	}
}

/* Select K training points that are closest to the testing point */
void selectK(int point)
{
	map<double, int>::iterator it = dist.begin();
	for (int i = 0; i < K; i++)
	{
		selected[i] = training[it->second];
		it++;
	}
}

/* Decide which class the testing point belongs to */
void decideClass(int point)
{
	int irisClass[3] = {0};			// Represent three different iris classes: 1-Setosa 2-Versicolor 3-Virginica
	int irisClassMax = 0, classNum = -1;
	for (int i = 0; i < K; i++)
	{
		if (selected[i].classBelong == "Iris-setosa")
			irisClass[0]++;
		else if (selected[i].classBelong == "Iris-versicolor")
			irisClass[1]++;
		else if (selected[i].classBelong == "Iris-virginica")
			irisClass[2]++;
	}
	for (int i = 0; i < 3; i++)
	{
		if (irisClassMax < irisClass[i])
		{
			irisClassMax = irisClass[i];
			classNum = i;
		}
	}
	switch (classNum)
	{
		case 0:
			testing[point].classBelong = "Iris-setosa";
			break;
		case 1:
			testing[point].classBelong = "Iris-versicolor";
			break;
		case 2:
			testing[point].classBelong = "Iris-virginica";
			break;
		default:
			cout << "Class of testing[point] unknown." << endl;
	}
}

int main()
{
	fstream inout;
	inout.open("bezdekIris.data", ios::in);
	for (int i = 0; i < SIZE; i++)
	{
		inout >> training[i].sepalLength >> training[i].sepalWidth >> training[i].petalLength >> training[i].petalWidth >> training[i].classBelong;
	}
	inout.close();
	inout.open("iris.data", ios::in);
	for (int i = 0; i < SIZE; i++)
	{
		string tmp;
		inout >> testing[i].sepalLength >> testing[i].sepalWidth >> testing[i].petalLength >> testing[i].petalWidth >> tmp;
	}
	inout.close();

	for (int i = 0; i < SIZE; i++)
	{
		computeDist(i);
		selectK(i);
		decideClass(i);
	}

	inout.open("res.txt", ios::out);
	for (int i = 0; i < SIZE; i++)
	{
		inout << testing[i].classBelong << endl;
	}
	inout.close();

	system("pause");
	return 0;
}