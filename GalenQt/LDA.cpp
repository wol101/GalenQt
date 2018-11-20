#include "LDA.h"

#include <Eigen/Dense>

#include <map>
#include <vector>
#include <iostream>

#define DEBUG_LDA 1
#ifdef QT_NO_DEBUG_OUTPUT
#undef DEBUG_LDA
#define DEBUG_LDA 0
#endif

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
#if DEBUG_LDA
    std::cerr << "x" << "\n" << x << "\n";
    std::cerr << "labels" << "\n" << labels << "\n";
#endif
    if (x.cols() < 2 && x.rows() < 2) return 1;
    if (x.rows() != labels.rows()) return 1;

    m_labels = labels;
    m_ncols = x.cols();
    m_nrows = x.rows();

    if (m_isScale)
    {
        if (m_isCenter)
        {
            Eigen::RowVectorXf mean = x.colwise().mean();
            Eigen::RowVectorXf std = ((x.rowwise() - mean).array().square().colwise().sum() / (x.rows() - 1)).sqrt();
            m_x = (x.rowwise() - mean).array().rowwise() / std.array();
            m_sd_vector = std;
            m_mean_vector = mean;
        }
        else
        {
            Eigen::RowVectorXf mean = x.colwise().mean();
            Eigen::RowVectorXf std = ((x.rowwise() - mean).array().square().colwise().sum() / (x.rows() - 1)).sqrt();
            m_sd_vector = std;
            m_x = x.array().rowwise() / std.array();
        }
    }
    else
    {
        if (m_isCenter)
        {
            Eigen::RowVectorXf mean = x.colwise().mean();
            m_x = x.rowwise() - mean;
            m_mean_vector = mean;
        }
        else
        {
            m_x = x;
        }
    }

    // inspired by Chapter 6 of Machine Learning: An Algorithmic Perspective (2nd Edition)
    // by Stephen Marsland (http://stephenmonika.net)

    // Intialize Sw
    Eigen::MatrixXf Sw = Eigen::MatrixXf::Zero(m_ncols, m_ncols);

    // Compute total covariance matrix
    Eigen::MatrixXf St = covariance(m_x);

    // now split up the data into classes
    std::map<int, int> labelsMap;
    for (unsigned int i = 0; i < m_nrows; i++) labelsMap[m_labels(i)] += 1; // containers initialise primitive types to zero
    // now I know what classes I've got and how big they each are, I can preallocate the storage
    std::map<int, Eigen::MatrixXf *> dataPerClassList;
    std::map<int, int> nextIndexList;
    Eigen::MatrixXf *dataPerClass;
    int rowIndex;
    for (std::map<int, int>::iterator iter = labelsMap.begin(); iter != labelsMap.end(); iter++)
    {
        dataPerClass = new Eigen::MatrixXf(iter->second, m_ncols);
        dataPerClassList[iter->first] = dataPerClass;
        nextIndexList[iter->first] = 0;
    }
    // and this copies the correct data into the correct matrix
    for (unsigned int i = 0; i < m_nrows; i++)
    {
        dataPerClass = dataPerClassList[m_labels(i)];
        rowIndex = nextIndexList[m_labels(i)];
        dataPerClass->row(rowIndex) = m_x.row(i);
        nextIndexList[m_labels(i)]++;
    }

    // Sum over classes
    for (std::map<int, Eigen::MatrixXf *>::iterator iter = dataPerClassList.begin(); iter != dataPerClassList.end(); iter++)
    {
        dataPerClass = iter->second;
        Eigen::MatrixXf C = covariance(*dataPerClass);
        float p = float(dataPerClass->rows()) / float((m_nrows));
        Sw += (p * C);
    }

    // Compute between class scatter
    Eigen::MatrixXf Sb = St - Sw;

    // Perform eigendecomposition of inv(Sw)*Sb
    // Sw and Sb should be symmetric and real and hence self-adjoint
    // which means we can use the GeneralizedSelfAdjointEigenSolver instead of GeneralizedEigenSolver
    // and we do not have to worry about real and imaginary components of the eigenvectors
    Eigen::GeneralizedSelfAdjointEigenSolver<Eigen::MatrixXf> ges;
    ges.compute(Sb, Sw); // the order of Sb and Sw is important

    // Sort eigenvalues and eigenvectors in descending order
    typedef std::pair<float, int> eigen_pair;
    std::vector<eigen_pair> ep(m_ncols);
    for (unsigned int i = 0 ; i < m_ncols; i++) ep[i] = std::make_pair(ges.eigenvalues()[i], i);
    std::sort(ep.begin(), ep.end()); // Ascending order by default
    // Sort them all in descending order
    m_eigenvectors_sorted = Eigen::MatrixXf::Zero(ges.eigenvectors().rows(), ges.eigenvectors().cols());
    m_eigenvalues_sorted = Eigen::VectorXf::Zero(m_ncols);
    for (std::size_t i = 0; i < ep.size(); i++)
    {
        m_eigenvalues_sorted(ep.size() - i - 1) = ep[i].first;
        m_eigenvectors_sorted.col(ep.size() - i - 1) += ges.eigenvectors().col(ep[i].second);
    }

    // Compute mapped data
    m_scores = m_x * m_eigenvectors_sorted;

    m_sd = Eigen::VectorXf(m_ncols);
    m_prop_of_var = Eigen::VectorXf(m_ncols);
    m_kaiser = 0;
    float tmp_sum = m_eigenvalues_sorted.sum();
    for (unsigned int i = 0; i < m_ncols; i++)
    {
        m_sd(i) = sqrt(m_eigenvalues_sorted(i));
        if (m_sd(i) >= 1) m_kaiser = i + 1;
        m_prop_of_var(i) = m_eigenvalues_sorted(i)/tmp_sum;
    }

    // PC's cumulative proportion
    m_thresh95 = 1;
    m_cum_prop = Eigen::VectorXf(m_ncols);
    m_cum_prop(0) = m_prop_of_var(0);
    for (unsigned int i = 1; i < m_prop_of_var.size(); i++)
    {
        m_cum_prop(i) = m_cum_prop(i - 1) + m_prop_of_var(i);
        if (m_cum_prop(i) < 0.95f) m_thresh95 = i + 1;
    }

    m_scores = m_x * m_eigenvectors_sorted;

#if DEBUG_LDA
    std::cerr << "m_x" << "\n" << m_x << "\n";
    std::cerr << "Sw (covariance within classes)" << "\n" << Sw << "\n";
    std::cerr << "Sb (covariance between classes)" << "\n" << Sb << "\n";
    std::cerr << "m_eigenvalues_sorted" << "\n" << m_eigenvalues_sorted << "\n";
    std::cerr << "m_eigenvectors_sorted" << "\n" << m_eigenvectors_sorted << "\n";
    std::cerr << "m_sd" << "\n" << m_sd << "\n";
    std::cerr << "m_prop_of_var" << "\n" << m_prop_of_var << "\n";
    std::cerr << "m_cum_prop" << "\n" << m_cum_prop << "\n";
    std::cerr << "m_scores" << "\n" << m_scores << "\n";
#endif

    return 0;
}

int LDA::Extend(const Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> &x)
{
    if (m_isScale)
    {
        if (m_isCenter)
        {
            m_extended = (x.rowwise() - m_mean_vector).array().rowwise() / m_sd_vector.array();
        }
        else
        {
            m_extended = x.array().rowwise() / m_sd_vector.array();
        }
    }
    else
    {
        if (m_isCenter)
        {
            m_extended = x.rowwise() - m_mean_vector;
        }
        else
        {
            m_extended = x;
        }
    }

    m_extendedScores = m_extended * m_eigenvectors_sorted;

    return 0;
}

Eigen::MatrixXf LDA::covariance(const Eigen::MatrixXf &mat)
{
    Eigen::MatrixXf centered = mat.rowwise() - mat.colwise().mean();
    Eigen::MatrixXf cov = (centered.adjoint() * centered) / float(mat.rows() - 1);
    return cov;
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


