#include "PCA.h"

#include <Eigen/SVD>

#include <vector>

PCA::PCA()
{
    m_nrows = 0;
    m_ncols = 0;
    m_isCenter = true;
    m_isScale = false;
    m_isCorr = false;
    m_kaiser = 0;
    m_thresh95 = 1;
}

PCA::~PCA()
{
}

int PCA::Calculate(const Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> &x)
{
    m_x = x;
    m_ncols = m_x.cols();
    m_nrows = m_x.rows();
    if (m_ncols < 2 && m_nrows < 2) return 1;

    // Mean and standard deviation for each column
    Eigen::VectorXf mean_vector(m_ncols);
    mean_vector = m_x.colwise().mean();
    Eigen::VectorXf sd_vector(m_ncols);
    float denom = static_cast<float>(m_nrows - 1);
    for (unsigned int i = 0; i < m_ncols; ++i)
    {
        Eigen::VectorXf curr_col  = Eigen::VectorXf::Constant(m_nrows, mean_vector(i)); // mean(x) for column x
        curr_col = m_x.col(i) - curr_col; // x - mean(x)
        curr_col = curr_col.array().square(); // (x-mean(x))^2
        sd_vector(i) = sqrt((curr_col.sum())/denom);
        if (sd_vector(i) == 0) return 2;
    }

    // Shift to zero
    if (true == m_isCenter)
    {
        for (unsigned int i = 0; i < m_ncols; ++i)
        {
            m_x.col(i) -= Eigen::VectorXf::Constant(m_nrows, mean_vector(i));
        }
    }

    // Scale to unit variance
    if (true == m_isScale)
    {
        for (unsigned int i = 0; i < m_ncols; ++i)
        {
            m_x.col(i) = m_x.col(i).array() / Eigen::ArrayXf::Constant(m_nrows, sd_vector(i));
        }
    }

    if (m_isCorr == false)
    {
        // Singular Value Decomposition (the commonest version)
        Eigen::JacobiSVD<Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor>> svd(m_x, Eigen::ComputeThinV);
        Eigen::VectorXf eigen_singular_values = svd.singularValues();
        Eigen::VectorXf tmp_vec = eigen_singular_values.array().square();
        float tmp_sum = tmp_vec.sum();
        tmp_vec /= tmp_sum;
        // PC's standard deviation and
        // PC's proportion of variance
        m_prop_of_var = tmp_vec;
        m_sd = eigen_singular_values / sqrt(denom);

        // the Kaiser criterion (the last component with eigenvalue greater than 1)
        m_kaiser = 0;
        for (unsigned int i = 0; i < m_ncols; ++i)
        {
            if (m_sd(i) >= 1) m_kaiser = i + 1;
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

        // Scores
        m_scores = m_x * svd.matrixV();
    }
    else
    {
        // Calculate covariance matrix
        Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> eigen_cov;
        Eigen::VectorXf sds;
        // (TODO) Should be weighted cov matrix, even if is_center == false
        eigen_cov = (1.0 /(m_nrows/*-1*/)) * m_x.transpose() * m_x;
        sds = eigen_cov.diagonal().array().sqrt();
        Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> outer_sds = sds * sds.transpose();
        eigen_cov = eigen_cov.array() / outer_sds.array();
        // ?If data matrix is scaled, covariance matrix is equal to correlation matrix
        Eigen::EigenSolver<Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor>> edc(eigen_cov);
        Eigen::VectorXf eigen_eigenvalues = edc.eigenvalues().real();
        Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> eigen_eigenvectors = edc.eigenvectors().real();

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
        Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> eigen_eigenvectors_sorted = Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor>::Zero(eigen_eigenvectors.rows(), eigen_eigenvectors.cols());
        Eigen::VectorXf eigen_eigenvalues_sorted = Eigen::VectorXf::Zero(m_ncols);
        int colnum = 0;
        for (int i = ep.size() - 1; i > -1; i--)
        {
            eigen_eigenvalues_sorted(colnum) = ep[i].first;
            eigen_eigenvectors_sorted.col(colnum++) += eigen_eigenvectors.col(ep[i].second);
        }

        m_sd = Eigen::VectorXf(m_ncols);
        m_prop_of_var = Eigen::VectorXf(m_ncols);
        m_kaiser = 0;
        float tmp_sum = eigen_eigenvalues_sorted.sum();
        for (unsigned int i = 0; i < m_ncols; ++i)
        {
            m_sd(i) = sqrt(eigen_eigenvalues_sorted(i));
            if (m_sd(i) >= 1) m_kaiser = i + 1;
            m_prop_of_var(i) = eigen_eigenvalues_sorted(i)/tmp_sum;
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

        // Scores for PCA with correlation matrix
        // Scale before calculating new values
        for (unsigned int i = 0; i < m_ncols; ++i)
        {
            m_x.col(i) /= sds(i);
        }

        m_scores = m_x * eigen_eigenvectors_sorted;
    }
    return 0;
}

Eigen::VectorXf PCA::sd() const
{
    return m_sd;
}

Eigen::VectorXf PCA::prop_of_var() const
{
    return m_prop_of_var;
}

Eigen::VectorXf PCA::cum_prop() const
{
    return m_cum_prop;
}

Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> *PCA::scores()
{
    return &m_scores;
}

unsigned int PCA::kaiser() const
{
    return m_kaiser;
}

unsigned int PCA::thresh95() const
{
    return m_thresh95;
}

void PCA::setIsCenter(bool isCenter)
{
    m_isCenter = isCenter;
}

void PCA::setIsScale(bool isScale)
{
    m_isScale = isScale;
}

void PCA::setIsCorr(bool isCorr)
{
    m_isCorr = isCorr;
}




