// main.cpp

#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cassert>
#include <algorithm>

bool ReadLine(const std::string &line, std::vector<double> &lineData)
{
	std::istringstream ss(line);
	std::string token;

	while (std::getline(ss, token, ','))
	{
		std::istringstream tokenStream(token);
		double value;
		if ((tokenStream >> value).fail())
		{
			std::cerr << "Failed to convert '" << token << "' to a number\n";
			return false;
		}

		lineData.push_back(value);
	}

	return true;
}

bool ReadData(const std::string& inputFileName, std::vector<std::vector<double>>& inputData)
{
	std::ifstream inFile(inputFileName.c_str());
	if (!inFile.good() || !inFile.is_open())
	{
		std::cerr << "Failed to open '" << inputFileName << "' for input\n";
		return false;
	}

	unsigned int lineDataSize(0);
	unsigned int lineCount(0);

	std::string line;
	while (std::getline(inFile, line))
	{
		++lineCount;
		std::vector<double> lineData;
		if (!ReadLine(line, lineData))
		{
			std::cerr << "Failed to parse line " << lineCount << '\n';
			return false;
		}

		if (lineDataSize == 0)
			lineDataSize = lineData.size();
		else if (lineData.size() != lineDataSize)
		{
			std::cerr << "On line " << lineCount << ", found " << lineData.size() << " columns, expected " << lineDataSize << '\n';
			return false;
		}

		inputData.push_back(lineData);
	}

	return true;
}

double Interpolate(const double &x1, const double &x2, const double &x3, const double &y1, const double &y3)
{
	return (x2 - x1) / (x3 - x1) * (y3 - y1) + y1;
}

void Interpolate(const std::vector<double>& inSlice1, const std::vector<double>& inSlice2, std::vector<double>& outSlice)
{
	unsigned int j;
	for (j = 1; j < outSlice.size(); ++j)
		outSlice[j] = Interpolate(inSlice1.front(), outSlice.front(), inSlice2.front(), inSlice1[j], inSlice2[j]);
}

void AdjustInputIndex(unsigned int& i, const std::vector<std::vector<double>>& inputData, const double& time)
{
	while (i < inputData.size() - 2)
	{
		if (inputData[i].front() < time && inputData[i + 1].front() >= time)
			break;
		++i;
	}
}

void Resample(const std::vector<std::vector<double>>& inputData, std::vector<std::vector<double>>& outputData, const double& outputFrequency)
{
	const unsigned int columnCount(inputData.front().size());
	const double timeStep(1.0 / outputFrequency);// [sec]
	double time(inputData[0][0]);
	outputData.resize(static_cast<unsigned int>((inputData.back()[0] - inputData.front()[0]) * outputFrequency) + 1);
	std::for_each(outputData.begin(), outputData.end(), [&time, timeStep, columnCount](auto& slice)
	{
		slice.resize(columnCount);
		slice.front() = time;
		time += timeStep;
	});

	unsigned int i(0);
	for (auto& outSlice : outputData)
	{
		if (outSlice.front() == inputData[i].front())
			outSlice = inputData[i];
		else
		{
			AdjustInputIndex(i, inputData, outSlice.front());
			Interpolate(inputData[i], inputData[i + 1], outSlice);
		}
	}
}

bool WriteData(const std::string& outputFileName, const std::vector<std::vector<double>> &outputData)
{
	std::ofstream outFile(outputFileName.c_str());
	if (!outFile.is_open() || !outFile.good())
	{
		std::cerr << "Failed to open '" << outputFileName << "' for output\n";
		return false;
	}

	outFile.precision(14);
	for (const auto& entry : outputData)
	{
		for (const auto& column : entry)
			outFile << column << ',';
		outFile << '\n';
	}

	return true;
}

int main(int argc, char* argv[])
{
	if (argc != 4)
	{
		std::cout << "Usage = " << argv[0] << " <output frequency in Hz> <input file name> <output file name>\n";
		std::cout << "  First column of input file is assumed to be time in seconds." << std::endl;
		std::cout << "  Input file must not contain header rows." << std::endl;
		std::cout << "  Input file must be comma-separated." << std::endl;
		return 1;
	}

	double outputFrequency;
	std::istringstream ss(argv[1]);
	if ((ss >> outputFrequency).fail())
	{
		std::cerr << "Failed to convert '" << argv[1] << "' to output frequency\n";
		return 1;
	}

	const std::string inputFileName(argv[2]);
	const std::string outputFileName(argv[3]);

	std::vector<std::vector<double>> inputData;
	if (!ReadData(inputFileName, inputData))
		return 1;

	std::vector<std::vector<double>> outputData;
	Resample(inputData, outputData, outputFrequency);

	if (!WriteData(outputFileName, outputData))
		return 1;

	return 0;
}