#ifndef PCA_H_
#define PCA_H_

#include <Eigen/Dense>

class PCA
{
public:
    PCA();
    ~PCA();

//    Initializing values and performing PCA
//    x         Initial data matrix (rows are data e.g. pixel values; cols are dimensions e.g number of images)
//    is_corr   'PCA with correlation matrix instead of 'PCA with singular value decomposition'
//    is_center Whether the variables should be shifted to be zero centered
//    is_scale  Whether the variables should be scaled to have unit variance
//    returns 0 if OK

    int Calculate(const Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> &x);

    Eigen::VectorXf sd() const;

    Eigen::VectorXf prop_of_var() const;

    Eigen::VectorXf cum_prop() const;

    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> *scores();

    unsigned int kaiser() const;

    unsigned int thresh95() const;

    void setIsCenter(bool isCenter);

    void setIsScale(bool isScale);

    void setIsCorr(bool isCorr);

private:
    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> m_x; // Initial matrix as Eigen MatrixXf structure
    unsigned int  m_nrows;          // Number of rows in matrix x.
    unsigned int m_ncols;           // Number of cols in matrix x.
    bool  m_isCenter;               // Whether the variables should be shifted to be zero centered
    bool m_isScale;                 // Whether the variables should be scaled to have unit variance
    bool m_isCorr;                  // PCA with correlation matrix, not covariance
    Eigen::VectorXf m_sd;           // Standard deviation of each component
    Eigen::VectorXf m_prop_of_var;  // Proportion of variance
    Eigen::VectorXf m_cum_prop;     // Cumulative proportion
    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> m_scores; // Rotated values
    unsigned int  m_kaiser;         // Number of PC according Kaiser criterion (the last component with eigenvalue greater than 1)
    unsigned int m_thresh95;        // Number of PC according 95% variance threshold
};

#endif
