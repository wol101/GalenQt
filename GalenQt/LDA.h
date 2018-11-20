#ifndef LDA_H
#define LDA_H

#include <Eigen/Dense>

class LDA
{
public:
    LDA();
    ~LDA();

    int Calculate(const Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> &x, const Eigen::VectorXi &labels);
    int Extend(const Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> &x);

    Eigen::MatrixXf covariance(const Eigen::MatrixXf &mat);

    Eigen::VectorXf sd() const;

    Eigen::VectorXf prop_of_var() const;

    Eigen::VectorXf cum_prop() const;

    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> *scores();

    unsigned int kaiser() const;

    unsigned int thresh95() const;

    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> *eigenvectors_sorted();

    Eigen::VectorXf *eigenvalues_sorted();

    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> *extendedScores();

    void setIsCenter(bool isCenter);

    void setIsScale(bool isScale);

private:
    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> m_x; // Initial matrix as Eigen MatrixXf structure
    Eigen::VectorXi m_labels;       // Vector of integers labelling the data classes
    Eigen::Index  m_nrows;          // Number of rows in matrix x.
    Eigen::Index m_ncols;           // Number of cols in matrix x.
    bool  m_isCenter;              // Whether the variables should be shifted to be zero centered
    bool m_isScale;                // Whether the variables should be scaled to have unit variance
    Eigen::VectorXf m_sd;           // Standard deviation of each component
    Eigen::VectorXf m_prop_of_var;  // Proportion of variance
    Eigen::VectorXf m_cum_prop;     // Cumulative proportion
    unsigned int  m_kaiser;         // Number of PC according Kaiser criterion (the last component with eigenvalue greater than 1)
    unsigned int m_thresh95;        // Number of PC according 95% variance threshold
    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> m_scores; // Rotated values
    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> m_eigenvectors_sorted; // eigen vector matrix
    Eigen::VectorXf m_eigenvalues_sorted; // eigenvalue matrix
    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> m_extended; // Extended matrix as Eigen MatrixXf structure
    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> m_extendedScores; // Extended rotated values
    Eigen::RowVectorXf m_mean_vector;  // internal mean value used for centering
    Eigen::RowVectorXf m_sd_vector;   // internal sd value used for normalising

};

#endif // LDA_H
