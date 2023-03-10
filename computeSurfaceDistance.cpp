#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <stdlib.h>
#include "eigen/Eigen/Dense"

using namespace std;

//ASSUMPTIONS
//1. we won't try to query two contiguous pixels since that calculation is done more easily by hand
//2. this kernel method will work :)

vector<int> PointToGridIndeces(Eigen::Vector3d p);
double computeSurfaceDistances(int x1, int y1, int x2, int y2, vector<unsigned char>& dataPre, vector<unsigned char>& dataPost);
int getIndex(int x, int y);
void computeHeight(Eigen::Vector3d& p, double rp, vector<unsigned char>& data);

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

    //Query whichever points we want here!
    computeSurfaceDistances(0, 0, 511, 511, dataPre, dataPost); //diagonal across whole map
    computeSurfaceDistances(170, 170, 340, 340, dataPre, dataPost); //diagonal across middle 1/3rd diagonal
    computeSurfaceDistances(170, 340, 340, 340, dataPre, dataPost); //roughly straight across peak

    filePre.close();
    filePost.close();

    return 0;
}

/*=====================FUNCTIONS==========================*/

//Compute distance from A to B for pre and post eruption data, then print each and their difference!
double computeSurfaceDistances(int x1, int y1, int x2, int y2, vector<unsigned char>& dataPre, vector<unsigned char>& dataPost){

    cout << "Computing surface distance from pixel A = (" << x1 << "," << y1 << ") to pixel B = (" << x2 << ", " << y2 << ")" << endl;

    //Params
    double rp = 30.0 * sqrt(2);

    //-----Step 2: Generate points from A to B

    Eigen::Vector3d A((double)x1 * 30.0 + 15.0, (double)y1 * 30.0 + 15.0, 0.0); //compute 3D location of A (cells are 30 m wide, pixels are cell-centered) -- fill height later since different between maps
    Eigen::Vector3d B((double)x2 * 30.0 + 15.0, (double)y2 * 30.0 + 15.0, 0.0);

    //Now break line into segments of at LEAST length = rp = neighbor radius = 30 root 2
    //We want as many quadrature points as possible while maintaining segmentLength > rp
    Eigen::Vector3d direction = (B-A).normalized(); //normalized direction of path
    double length = (B-A).norm(); //length of path
    double segmentLength = length;
    int numSegments = 1;
    while(segmentLength > rp){
        numSegments++;
        segmentLength = length / (double)numSegments; 
    }

    //Construct path of points
    vector<Eigen::Vector3d> queryPoints;
    queryPoints.push_back(A);
    for(int i = 1; i < numSegments; i++){
        Eigen::Vector3d newP = A;
        newP += (direction * segmentLength * (double)i);
        queryPoints.push_back(newP);
    }
    queryPoints.push_back(B);

    //-----Step 3: Compute height at each point while racking up the surface distance as we go!
    
    //Get 1-D index for 2D pixel coordinates
    int idxA = getIndex(x1, y1);
    int idxB = getIndex(x2, y2);
    
    //First let's do this for PRE data
    double distancePre = 0.0;
    for(int i = 0; i < (int)queryPoints.size(); i++){
        if(i == 0){ //A
            queryPoints[0][2] = (double)dataPre[idxA] * 11.0; //directly set A's height from the pixel data
        }
        else if(i == numSegments){ //B
            queryPoints[i][2] = (double)dataPre[idxB] * 11.0; //directly set B's height from pixel data
            Eigen::Vector3d currPoint = queryPoints[i];
            Eigen::Vector3d prevPoint = queryPoints[i-1];
            distancePre += (currPoint - prevPoint).norm();
        }
        else{
            //Compute Height Using Kernel
            Eigen::Vector3d currPoint = queryPoints[i];
            Eigen::Vector3d prevPoint = queryPoints[i-1];

            computeHeight(currPoint, rp, dataPre);
            queryPoints[i] = currPoint; //update this point to include height! because we grab i-1 to compare
            
            distancePre += (currPoint - prevPoint).norm();
        }
    }

    cout << "Surface Distance Pre-Eruption: " << distancePre << endl;

    //Now for the POST data!
    double distancePost = 0.0;
    for(int i = 0; i < (int)queryPoints.size(); i++){
        if(i == 0){ //A
            queryPoints[0][2] = (double)dataPost[idxA] * 11.0; //directly set A's height from the pixel data
        }
        else if(i == numSegments){ //B
            queryPoints[i][2] = (double)dataPost[idxB] * 11.0; //directly set B's height from pixel data
            Eigen::Vector3d currPoint = queryPoints[i];
            Eigen::Vector3d prevPoint = queryPoints[i-1];
            distancePost += (currPoint - prevPoint).norm();
        }
        else{
            //Compute Height Using Kernel
            Eigen::Vector3d currPoint = queryPoints[i];
            Eigen::Vector3d prevPoint = queryPoints[i-1];

            computeHeight(currPoint, rp, dataPost);
            queryPoints[i] = currPoint; //update this point to include height! because we grab i-1 to compare
            
            distancePost += (currPoint - prevPoint).norm();
        }
    }

    cout << "Surface Distance Post-Eruption: " << distancePost << endl;

    cout << "Distance Post - Distance Pre: " << distancePost - distancePre << endl << endl;
}

int getIndex(int x, int y){
    return x + (y * 512);
}

// Inspired by the scalar field construction done by Homel and Herbold 2016 to compute damage gradients in MPM
// Free PDF on ResearchGate: https://www.researchgate.net/publication/303917651_Field-Gradient_Partitioning_for_Fracture_and_Frictional_Contact_in_the_Material_Point_Method
// Check Equations 17 to 19
void computeHeight(Eigen::Vector3d& p, double rp, vector<unsigned char>& data){
    vector<int> idx;
    idx = PointToGridIndeces(p); //get 2-D grid index --> which pixel is our query point inside?
    int i = idx[0];
    int j = idx[1];
    double Hx = 0; //field num, H(x)
    double Sx = 0; //field denom, S(x)
    for(int r = i-2; r < i+3; r++){ //iterate the 5x5 stencil of neighbor cells 
        for(int s = j-2; s < j+2; s++){
            
            //index filtering --> NO TOROIDAL BEHAVIOR HERE
            if(r > 511 || s > 511 || r < 0 || s < 0){
                continue;
            }

            Eigen::Vector3d pixelPos((double)r * 30.0 + 15.0, (double)s * 30.0 + 15.0, 0.0);
            int pixelIdx = getIndex(r, s);
            double rBar = (p - pixelPos).norm() / rp;
            double omega = 1 - (3 * rBar * rBar) + (2 * rBar * rBar * rBar);
            if(rBar > 1.0){
                omega = 0.0; //distance check is baked into rBar
            }
            double pixelHeight = (double)data[pixelIdx] * 11.0;

            Hx += pixelHeight * omega;
            Sx += omega;
        }
    }
    if(Sx > 0){
        p[2] = Hx / Sx; //set height in the point
    }
    
    return;
}

//What pixel coordinate does this particle lie in?
vector<int> PointToGridIndeces(Eigen::Vector3d p){
    vector<int> idx(2,-1);
    int i, j = 0;
    i = floor(p[0]/30.0);
    j = floor(p[1]/30.0);
    idx[0] = i;
    idx[1] = j;
    return idx;
}
