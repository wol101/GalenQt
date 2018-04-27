#include "LDA.h"

#include <Eigen/Dense>

#include <map>
#include <vector>
#include <iostream>

#define DEBUG_LDA

LDA::LDA()
{
    m_nrows = 0;
    m_ncols = 0;
    m_isCenter = true;
    m_isScale = false;
    m_kaiser = 0;
    m_thresh95 = 1;
}

LDA::~LDA()
{
}

int LDA::Calculate(const Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> &x, const Eigen::VectorXi &labels)
{
    m_x = x;
    m_labels = labels;
    m_ncols = m_x.cols();
    m_nrows = m_x.rows();
    if (m_x.cols() < 2 && m_x.rows() < 2) return 1;
    if (m_x.rows() != m_labels.rows()) return 1;

    // Mean and standard deviation for each column
    m_mean_vector = Eigen::VectorXf(m_ncols);
    m_mean_vector = m_x.colwise().mean();
    m_sd_vector = Eigen::VectorXf(m_ncols);
    float denom = static_cast<float>(m_nrows - 1);
    for (unsigned int i = 0; i < m_ncols; ++i)
    {
        Eigen::VectorXf curr_col  = Eigen::VectorXf::Constant(m_nrows, m_mean_vector(i)); // mean(x) for column x
        curr_col = m_x.col(i) - curr_col; // x - mean(x)
        curr_col = curr_col.array().square(); // (x-mean(x))^2
        m_sd_vector(i) = sqrt((curr_col.sum())/denom);
        if (m_sd_vector(i) == 0) return 2;
    }

    // Shift to zero
    if (true == m_isCenter)
    {
        for (unsigned int i = 0; i < m_ncols; ++i)
        {
            m_x.col(i) -= Eigen::VectorXf::Constant(m_nrows, m_mean_vector(i));
        }
    }

    // Scale to unit variance
    if (true == m_isScale)
    {
        for (unsigned int i = 0; i < m_ncols; ++i)
        {
            m_x.col(i) = m_x.col(i).array() / Eigen::ArrayXf::Constant(m_nrows, m_sd_vector(i));
        }
    }

    // next 2 lines calculates the covariance matrix
    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> centered = m_x.rowwise() - m_x.colwise().mean();
    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> cov = (centered.adjoint() * centered) / double(centered.rows() - 1);

    // now split up the data into classes
    std::map<int, int> labelsMap;
    for(unsigned int i = 0; i < m_nrows; i++)
    {
        labelsMap[m_labels(i)] += 1; // containers initialise primitive types to zero
    }
    // now I know what classes I've got and how big they each are, I can preallocate the storage
    std::vector<Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> *> dataPerClassList;
    std::vector<int> nextIndexList;
    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> *dataPerClass;
    int rowIndex;
    for (std::map<int, int>::iterator iter = labelsMap.begin(); iter != labelsMap.end(); iter++)
    {
        dataPerClass = new Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor>(iter->second, m_ncols);
        dataPerClassList.push_back(dataPerClass);
        nextIndexList.push_back(0);
    }
    // and this copies the correct data into the correct matrix
    for(unsigned int i = 0; i < m_nrows; i++)
    {
        dataPerClass = dataPerClassList[m_labels(i)];
        rowIndex = nextIndexList[m_labels(i)];
        dataPerClass->row(rowIndex) = m_x.row(i);
        nextIndexList[m_labels(i)]++;
    }
    // now we can do the real work of calculating the individual covarient matrices
    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> sw = Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor>::Zero(m_ncols, m_ncols);
    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> classCov;
    for(unsigned int i = 0; i < dataPerClassList.size(); i++)
    {
        dataPerClass = dataPerClassList[i];
        // next 2 lines calculates the covariance matrix
        centered = (*dataPerClass).rowwise() - (*dataPerClass).colwise().mean();
        classCov = (centered.adjoint() * centered) / double(centered.rows() - 1);
        float fac = float(dataPerClass->rows()) / float(m_nrows);
        sw += (classCov.array() * fac).matrix();
    }
    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> sb = cov - sw;

    // Now solve for W and compute mapped data
    // Compute eigenvalues, eigenvectors and sort into order
    // sw and sb should be symmetric and real and hence self-adjoint
    // which means we can use the GeneralizedSelfAdjointEigenSolver
        Eigen::GeneralizedSelfAdjointEigenSolver<Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor>> ges;
        ges.compute(sw, sb);
        Eigen::VectorXf eigen_eigenvalues = ges.eigenvalues();
        Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> eigen_eigenvectors = ges.eigenvectors();
//    Eigen::GeneralizedEigenSolver<Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor>> ges;
//    ges.compute(sw, sb);
//    Eigen::VectorXf eigen_eigenvalues = ges.eigenvalues().real();
//    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> eigen_eigenvectors = ges.eigenvectors().real();

    // The eigenvalues and eigenvectors are not sorted in any particular order.
    // So, we should sort them
    typedef std::pair<float, int> eigen_pair;
    std::vector<eigen_pair> ep;
    for (unsigned int i = 0 ; i < m_ncols; ++i)
    {
        ep.push_back(std::make_pair(eigen_eigenvalues(i), i));
    }
    std::sort(ep.begin(), ep.end()); // Ascending order by default
    // Sort them all in descending order
    m_eigenvectors_sorted = Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor>::Zero(eigen_eigenvectors.rows(), eigen_eigenvectors.cols());
    m_eigenvalues_sorted = Eigen::VectorXf::Zero(m_ncols);
    int colnum = 0;
    for (int i = ep.size() - 1; i >= 0; i--)
    {
        m_eigenvalues_sorted(colnum) = ep[i].first;
        m_eigenvectors_sorted.col(colnum) += eigen_eigenvectors.col(ep[i].second);
        colnum++;
    }

    m_sd = Eigen::VectorXf(m_ncols);
    m_prop_of_var = Eigen::VectorXf(m_ncols);
    m_kaiser = 0;
    float tmp_sum = m_eigenvalues_sorted.sum();
    for (unsigned int i = 0; i < m_ncols; ++i)
    {
        m_sd(i) = sqrt(m_eigenvalues_sorted(i));
        if (m_sd(i) >= 1) m_kaiser = i + 1;
        m_prop_of_var(i) = m_eigenvalues_sorted(i)/tmp_sum;
    }

    // PC's cumulative proportion
    m_thresh95 = 1;
    m_cum_prop = Eigen::VectorXf(m_ncols);
    m_cum_prop(0) = m_prop_of_var(0);
    for (unsigned int i = 1; i < m_prop_of_var.size(); ++i)
    {
        m_cum_prop(i) = m_cum_prop(i - 1) + m_prop_of_var(i);
        if (m_cum_prop(i) < 0.95f) m_thresh95 = i + 1;
    }

    m_scores = m_x * m_eigenvectors_sorted;

#ifdef DEBUG_LDA
    std::cerr << "m_x = " << m_x << std::endl << std::endl;
    std::cerr << "sw (covariance within classes) = " << sw << std::endl << std::endl;
    std::cerr << "sb (covariance between classes) = " << sb << std::endl << std::endl;
    std::cerr << "m_eigenvalues_sorted = " << m_eigenvalues_sorted << std::endl << std::endl;
    std::cerr << "m_eigenvectors_sorted = " << m_eigenvectors_sorted << std::endl << std::endl;
    std::cerr << "m_sd = " << m_sd << std::endl << std::endl;
    std::cerr << "m_prop_of_var = " << m_prop_of_var << std::endl << std::endl;
    std::cerr << "m_cum_prop = " << m_cum_prop << std::endl << std::endl;
#endif

    return 0;
}

int LDA::Extend(const Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> &extended)
{
    m_extended = extended;

    // Shift to zero
    if (true == m_isCenter)
    {
        for (unsigned int i = 0; i < m_extended.cols(); ++i)
        {
            m_extended.col(i) -= Eigen::VectorXf::Constant(m_extended.rows(), m_mean_vector(i));
        }
    }

    // Scale to unit variance
    if (true == m_isScale)
    {
        for (unsigned int i = 0; i < m_extended.cols(); ++i)
        {
            m_extended.col(i) = m_extended.col(i).array() / Eigen::ArrayXf::Constant(m_extended.rows(), m_sd_vector(i));
        }
    }

    m_extendedScores = m_extended * m_eigenvectors_sorted;

    return 0;
}

Eigen::VectorXf LDA::sd() const
{
    return m_sd;
}

Eigen::VectorXf LDA::prop_of_var() const
{
    return m_prop_of_var;
}

Eigen::VectorXf LDA::cum_prop() const
{
    return m_cum_prop;
}

Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> *LDA::scores()
{
    return &m_scores;
}

unsigned int LDA::kaiser() const
{
    return m_kaiser;
}

unsigned int LDA::thresh95() const
{
    return m_thresh95;
}

Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> *LDA::eigenvectors_sorted()
{
    return &m_eigenvectors_sorted;
}

Eigen::VectorXf *LDA::eigenvalues_sorted()
{
    return &m_eigenvalues_sorted;
}

Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> *LDA::extendedScores()
{
    return &m_extendedScores;
}

void LDA::setIsCenter(bool isCenter)
{
    m_isCenter = isCenter;
}

void LDA::setIsScale(bool isScale)
{
    m_isScale = isScale;
}


