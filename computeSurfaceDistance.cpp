#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <stdlib.h>
#include "eigen/Eigen/Dense"

using namespace std;

Eigen::Vector3d v(1,2,3);

//vector<int> PointToGridIndeces(Eigen::Vector3d p);
float computeSurfaceDistances(int x1, int y1, int x2, int y2, vector<unsigned char>& dataPre, vector<unsigned char>& dataPost);

main(int argc, char* argv[]){
    
    //Step 1: Read in input data
    ifstream filePre("data/pre.data", ios::binary);
    ifstream filePost("data/post.data", ios::binary);
    if(!filePre.is_open() || !filePost.is_open()){
        cerr << "Failed to open files!" << endl;
        return 1;
    }

    vector<unsigned char> dataPre(262144);
    vector<unsigned char> dataPost(262144);
    filePre.read(reinterpret_cast<char*>(dataPre.data()), dataPre.size());      //reinterpret cast required by read function
    filePost.read(reinterpret_cast<char*>(dataPost.data()), dataPost.size());   //thanks to chatG2P for all the binary stream syntax i ever wanted <3

    if (filePre.gcount() != dataPre.size() || filePost.gcount() != dataPost.size()) {
        cerr << "Failed to read binary files!" << std::endl;
        return 1;
    }

    computeSurfaceDistances(5, 10, 20, 30, dataPre, dataPost);

    filePre.close();
    filePost.close();

    return 0;
}


float computeSurfaceDistances(int x1, int y1, int x2, int y2, vector<unsigned char>& dataPre, vector<unsigned char>& dataPost){

    //Step 2: Generate points from A to B

    //Step 3: Compute height at each point while racking up the surface distance as we go!

    for (const auto& value : dataPre) {
        std::cout << static_cast<unsigned>(value) << " ";
    }
    std::cout << std::endl;

}
