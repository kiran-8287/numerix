#include "numerix.h"
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static numerix_status bad(numerix_error *error, numerix_status status,
                          const char *message)
{
    numerix_error_set(error, status, message);
    return status;
}
static int vecok(const numerix_vector*v){size_t i;if(!v||!v->data||!v->size)return 0;for(i=0;i<v->size;i++)if(!isfinite(v->data[i]))return 0;return 1;}
static int matok(const numerix_matrix*m){size_t i,n;if(!m||!m->data||!m->rows||!m->columns||m->rows>SIZE_MAX/m->columns)return 0;n=m->rows*m->columns;for(i=0;i<n;i++)if(!isfinite(m->data[i]))return 0;return 1;}
numerix_status numerix_vector_create(size_t n,numerix_vector**out,numerix_error*e){numerix_vector*v;if(!out)return bad(e,NUMERIX_STATUS_INVALID_ARGUMENT,"vector output required");*out=NULL;if(!n||n>SIZE_MAX/sizeof(double))return bad(e,NUMERIX_STATUS_INVALID_ARGUMENT,"invalid vector size");v=malloc(sizeof(*v));if(!v)return bad(e,NUMERIX_STATUS_ALLOCATION_FAILURE,"vector allocation failed");v->data=calloc(n,sizeof(double));if(!v->data){free(v);return bad(e,NUMERIX_STATUS_ALLOCATION_FAILURE,"vector data allocation failed");}v->size=n;*out=v;numerix_error_clear(e);return NUMERIX_STATUS_SUCCESS;}
void numerix_vector_destroy(numerix_vector*v){if(v){free(v->data);free(v);}}
numerix_status numerix_vector_copy(const numerix_vector*a,numerix_vector**out,numerix_error*e){numerix_status s;if(!out)return bad(e,NUMERIX_STATUS_INVALID_ARGUMENT,"vector output required");*out=NULL;if(!vecok(a))return bad(e,NUMERIX_STATUS_INVALID_ARGUMENT,"invalid vector");s=numerix_vector_create(a->size,out,e);if(!s)memcpy((*out)->data,a->data,a->size*sizeof(double));return s;}
static numerix_status vbin(const numerix_vector*a,const numerix_vector*b,numerix_vector**out,numerix_error*e,double sign){size_t i;numerix_status s;if(!out)return bad(e,NUMERIX_STATUS_INVALID_ARGUMENT,"vector output required");*out=NULL;if(!vecok(a)||!vecok(b)||a->size!=b->size)return bad(e,NUMERIX_STATUS_INVALID_DIMENSIONS,"vectors must be valid and have equal sizes");s=numerix_vector_create(a->size,out,e);if(s)return s;for(i=0;i<a->size;i++){(*out)->data[i]=a->data[i]+sign*b->data[i];if(!isfinite((*out)->data[i])){numerix_vector_destroy(*out);*out=NULL;return bad(e,NUMERIX_STATUS_NUMERICAL_FAILURE,"vector operation overflow");}}return NUMERIX_STATUS_SUCCESS;}
numerix_status numerix_vector_add(const numerix_vector*a,const numerix_vector*b,numerix_vector**o,numerix_error*e){return vbin(a,b,o,e,1);}
numerix_status numerix_vector_subtract(const numerix_vector*a,const numerix_vector*b,numerix_vector**o,numerix_error*e){return vbin(a,b,o,e,-1);}
numerix_status numerix_vector_scale(const numerix_vector*a,double c,numerix_vector**o,numerix_error*e){size_t i;numerix_status s;if(!o)return bad(e,NUMERIX_STATUS_INVALID_ARGUMENT,"vector output required");*o=NULL;if(!vecok(a)||!isfinite(c))return bad(e,NUMERIX_STATUS_INVALID_ARGUMENT,"invalid vector or scalar");s=numerix_vector_create(a->size,o,e);if(s)return s;for(i=0;i<a->size;i++){(*o)->data[i]=a->data[i]*c;if(!isfinite((*o)->data[i])){numerix_vector_destroy(*o);*o=NULL;return bad(e,NUMERIX_STATUS_NUMERICAL_FAILURE,"vector scaling overflow");}}return NUMERIX_STATUS_SUCCESS;}
numerix_status numerix_vector_dot(const numerix_vector*a,const numerix_vector*b,double*out,numerix_error*e){size_t i;double sum=0,c=0,y,t;if(!vecok(a)||!vecok(b)||a->size!=b->size||!out)return bad(e,NUMERIX_STATUS_INVALID_DIMENSIONS,"invalid vectors or output");for(i=0;i<a->size;i++){y=a->data[i]*b->data[i]-c;t=sum+y;c=(t-sum)-y;sum=t;}if(!isfinite(sum))return bad(e,NUMERIX_STATUS_NUMERICAL_FAILURE,"dot product overflow");*out=sum;numerix_error_clear(e);return NUMERIX_STATUS_SUCCESS;}
double numerix_vector_norm2(const numerix_vector *vector)
{
    size_t index;
    double scale = 0.0;
    double sum_squares = 1.0;

    if (!vecok(vector)) {
        return NAN;
    }
    for (index = 0; index < vector->size; ++index) {
        double value = fabs(vector->data[index]);

        if (value != 0.0) {
            if (scale < value) {
                double ratio = scale / value;

                sum_squares = 1.0 + sum_squares * ratio * ratio;
                scale = value;
            } else {
                double ratio = value / scale;

                sum_squares += ratio * ratio;
            }
        }
    }
    return scale == 0.0 ? 0.0 : scale * sqrt(sum_squares);
}
double numerix_vector_norm_inf(const numerix_vector*a){size_t i;double m=0;if(!vecok(a))return NAN;for(i=0;i<a->size;i++)if(fabs(a->data[i])>m)m=fabs(a->data[i]);return m;}
numerix_status numerix_matrix_create(size_t r,size_t c,numerix_matrix**out,numerix_error*e){numerix_matrix*m;size_t n;if(!out)return bad(e,NUMERIX_STATUS_INVALID_ARGUMENT,"matrix output required");*out=NULL;if(!r||!c||r>SIZE_MAX/c||(n=r*c)>SIZE_MAX/sizeof(double))return bad(e,NUMERIX_STATUS_INVALID_DIMENSIONS,"invalid matrix dimensions");m=malloc(sizeof(*m));if(!m)return bad(e,NUMERIX_STATUS_ALLOCATION_FAILURE,"matrix allocation failed");m->data=calloc(n,sizeof(double));if(!m->data){free(m);return bad(e,NUMERIX_STATUS_ALLOCATION_FAILURE,"matrix data allocation failed");}m->rows=r;m->columns=c;*out=m;numerix_error_clear(e);return NUMERIX_STATUS_SUCCESS;}
void numerix_matrix_destroy(numerix_matrix*m){if(m){free(m->data);free(m);}}
numerix_status numerix_matrix_copy(const numerix_matrix*a,numerix_matrix**out,numerix_error*e){numerix_status s;if(!out)return bad(e,NUMERIX_STATUS_INVALID_ARGUMENT,"matrix output required");*out=NULL;if(!matok(a))return bad(e,NUMERIX_STATUS_INVALID_ARGUMENT,"invalid matrix");s=numerix_matrix_create(a->rows,a->columns,out,e);if(!s)memcpy((*out)->data,a->data,a->rows*a->columns*sizeof(double));return s;}
static numerix_status mbin(const numerix_matrix*a,const numerix_matrix*b,numerix_matrix**out,numerix_error*e,double sign){size_t i,n;numerix_status s;if(!out)return bad(e,NUMERIX_STATUS_INVALID_ARGUMENT,"matrix output required");*out=NULL;if(!matok(a)||!matok(b)||a->rows!=b->rows||a->columns!=b->columns)return bad(e,NUMERIX_STATUS_INVALID_DIMENSIONS,"matrices must have equal dimensions");s=numerix_matrix_create(a->rows,a->columns,out,e);if(s)return s;n=a->rows*a->columns;for(i=0;i<n;i++){(*out)->data[i]=a->data[i]+sign*b->data[i];if(!isfinite((*out)->data[i])){numerix_matrix_destroy(*out);*out=NULL;return bad(e,NUMERIX_STATUS_NUMERICAL_FAILURE,"matrix operation overflow");}}return NUMERIX_STATUS_SUCCESS;}
numerix_status numerix_matrix_add(const numerix_matrix*a,const numerix_matrix*b,numerix_matrix**o,numerix_error*e){return mbin(a,b,o,e,1);}
numerix_status numerix_matrix_subtract(const numerix_matrix*a,const numerix_matrix*b,numerix_matrix**o,numerix_error*e){return mbin(a,b,o,e,-1);}
numerix_status numerix_matrix_scale(const numerix_matrix*a,double c,numerix_matrix**out,numerix_error*e){size_t i,n;numerix_status s;if(!out)return bad(e,NUMERIX_STATUS_INVALID_ARGUMENT,"matrix output required");*out=NULL;if(!matok(a)||!isfinite(c))return bad(e,NUMERIX_STATUS_INVALID_ARGUMENT,"invalid matrix or scalar");s=numerix_matrix_create(a->rows,a->columns,out,e);if(s)return s;n=a->rows*a->columns;for(i=0;i<n;i++){(*out)->data[i]=a->data[i]*c;if(!isfinite((*out)->data[i])){numerix_matrix_destroy(*out);*out=NULL;return bad(e,NUMERIX_STATUS_NUMERICAL_FAILURE,"matrix scaling overflow");}}return NUMERIX_STATUS_SUCCESS;}
numerix_status numerix_matrix_multiply(const numerix_matrix*a,const numerix_matrix*b,numerix_matrix**out,numerix_error*e){size_t i,j,k;numerix_status s;if(!out)return bad(e,NUMERIX_STATUS_INVALID_ARGUMENT,"matrix output required");*out=NULL;if(!matok(a)||!matok(b)||a->columns!=b->rows)return bad(e,NUMERIX_STATUS_INVALID_DIMENSIONS,"incompatible matrix dimensions");s=numerix_matrix_create(a->rows,b->columns,out,e);if(s)return s;for(i=0;i<a->rows;i++)for(k=0;k<a->columns;k++)for(j=0;j<b->columns;j++)(*out)->data[i*b->columns+j]+=a->data[i*a->columns+k]*b->data[k*b->columns+j];for(i=0;i<a->rows*b->columns;i++)if(!isfinite((*out)->data[i])){numerix_matrix_destroy(*out);*out=NULL;return bad(e,NUMERIX_STATUS_NUMERICAL_FAILURE,"matrix product overflow");}return NUMERIX_STATUS_SUCCESS;}
numerix_status numerix_matrix_transpose(const numerix_matrix*a,numerix_matrix**out,numerix_error*e){size_t i,j;numerix_status s;if(!out)return bad(e,NUMERIX_STATUS_INVALID_ARGUMENT,"matrix output required");*out=NULL;if(!matok(a))return bad(e,NUMERIX_STATUS_INVALID_ARGUMENT,"invalid matrix");s=numerix_matrix_create(a->columns,a->rows,out,e);if(s)return s;for(i=0;i<a->rows;i++)for(j=0;j<a->columns;j++)(*out)->data[j*a->rows+i]=a->data[i*a->columns+j];return NUMERIX_STATUS_SUCCESS;}
numerix_status numerix_matrix_vector_multiply(const numerix_matrix*a,const numerix_vector*x,numerix_vector**out,numerix_error*e){size_t i,j;numerix_status s;if(!out)return bad(e,NUMERIX_STATUS_INVALID_ARGUMENT,"output required");*out=NULL;if(!matok(a)||!vecok(x)||a->columns!=x->size)return bad(e,NUMERIX_STATUS_INVALID_DIMENSIONS,"incompatible matrix and vector");s=numerix_vector_create(a->rows,out,e);if(s)return s;for(i=0;i<a->rows;i++){for(j=0;j<a->columns;j++)(*out)->data[i]+=a->data[i*a->columns+j]*x->data[j];if(!isfinite((*out)->data[i])){numerix_vector_destroy(*out);*out=NULL;return bad(e,NUMERIX_STATUS_NUMERICAL_FAILURE,"matrix-vector product overflow");}}return NUMERIX_STATUS_SUCCESS;}

static numerix_status lu_raw(const numerix_matrix *matrix, double absolute_tolerance,
                             numerix_lu *lu, numerix_error *error)
{
    size_t dimension;
    size_t row;
    size_t column;
    size_t pivot_column;
    size_t pivot_row;
    double maximum = 0.0;

    (void)absolute_tolerance;
    memset(lu, 0, sizeof(*lu));
    if (!matok(matrix) || matrix->rows != matrix->columns) {
        return bad(error, NUMERIX_STATUS_INVALID_DIMENSIONS,
                   "LU requires a square matrix");
    }
    dimension = matrix->rows;
    if (dimension > SIZE_MAX / sizeof(double) ||
        dimension > SIZE_MAX / sizeof(size_t) ||
        dimension > SIZE_MAX / dimension ||
        dimension * dimension > SIZE_MAX / sizeof(double)) {
        return bad(error, NUMERIX_STATUS_INVALID_DIMENSIONS, "matrix too large");
    }
    lu->data = malloc(dimension * dimension * sizeof(double));
    lu->permutation = malloc(dimension * sizeof(size_t));
    if (lu->data == NULL || lu->permutation == NULL) {
        numerix_lu_destroy(lu);
        return bad(error, NUMERIX_STATUS_ALLOCATION_FAILURE, "LU allocation failed");
    }
    memcpy(lu->data, matrix->data, dimension * dimension * sizeof(double));
    lu->size = dimension;
    lu->sign = 1;
    for (row = 0; row < dimension * dimension; ++row) {
        if (fabs(matrix->data[row]) > maximum) {
            maximum = fabs(matrix->data[row]);
        }
    }
    lu->threshold = fmax(DBL_MIN, DBL_EPSILON * (double)dimension * maximum);
    for (row = 0; row < dimension; ++row) {
        lu->permutation[row] = row;
    }
    for (pivot_column = 0; pivot_column < dimension; ++pivot_column) {
        pivot_row = pivot_column;
        for (row = pivot_column + 1U; row < dimension; ++row) {
            if (fabs(lu->data[row * dimension + pivot_column]) >
                fabs(lu->data[pivot_row * dimension + pivot_column])) {
                pivot_row = row;
            }
        }
        if (fabs(lu->data[pivot_row * dimension + pivot_column]) <= lu->threshold) {
            numerix_lu_destroy(lu);
            return bad(error, NUMERIX_STATUS_SINGULAR_MATRIX,
                       "matrix has a zero or numerically negligible pivot");
        }
        if (pivot_row != pivot_column) {
            size_t permutation;

            for (column = 0; column < dimension; ++column) {
                double element = lu->data[pivot_column * dimension + column];

                lu->data[pivot_column * dimension + column] =
                    lu->data[pivot_row * dimension + column];
                lu->data[pivot_row * dimension + column] = element;
            }
            permutation = lu->permutation[pivot_column];
            lu->permutation[pivot_column] = lu->permutation[pivot_row];
            lu->permutation[pivot_row] = permutation;
            lu->sign = -lu->sign;
        }
        for (row = pivot_column + 1U; row < dimension; ++row) {
            lu->data[row * dimension + pivot_column] /=
                lu->data[pivot_column * dimension + pivot_column];
            for (column = pivot_column + 1U; column < dimension; ++column) {
                lu->data[row * dimension + column] -=
                    lu->data[row * dimension + pivot_column] *
                    lu->data[pivot_column * dimension + column];
            }
        }
    }
    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;
}
void numerix_lu_destroy(numerix_lu*l){if(l){free(l->data);free(l->permutation);memset(l,0,sizeof(*l));}}
numerix_status numerix_lu_decompose(const numerix_matrix*a,const numerix_options*o,numerix_lu*l,numerix_error*e){numerix_options opt;numerix_status s;if(!l)return bad(e,NUMERIX_STATUS_INVALID_ARGUMENT,"LU output is required");memset(l,0,sizeof(*l));if(o)opt=*o;else numerix_options_init(&opt);s=numerix_options_validate(&opt,e);if(s)return s;return lu_raw(a,opt.absolute_tolerance,l,e);}
numerix_status numerix_lu_solve(const numerix_lu *lu, const numerix_vector *right,
                               numerix_vector **solution, numerix_error *error)
{
    size_t row;
    size_t column;
    size_t dimension;
    numerix_status status;

    if (solution == NULL) {
        return bad(error, NUMERIX_STATUS_INVALID_ARGUMENT, "solution output required");
    }
    *solution = NULL;
    if (lu == NULL || lu->data == NULL || lu->permutation == NULL ||
        !vecok(right) || right->size != lu->size) {
        return bad(error, NUMERIX_STATUS_INVALID_DIMENSIONS,
                   "invalid LU decomposition or right-hand side");
    }
    dimension = lu->size;
    for (row = 0; row < dimension; ++row) {
        if (lu->permutation[row] >= dimension) {
            return bad(error, NUMERIX_STATUS_INVALID_INPUT, "invalid LU permutation");
        }
    }
    status = numerix_vector_create(dimension, solution, error);
    if (status != NUMERIX_STATUS_SUCCESS) {
        return status;
    }
    for (row = 0; row < dimension; ++row) {
        double value = right->data[lu->permutation[row]];

        for (column = 0; column < row; ++column) {
            value -= lu->data[row * dimension + column] * (*solution)->data[column];
        }
        (*solution)->data[row] = value;
    }
    for (row = dimension; row-- > 0U;) {
        double value = (*solution)->data[row];

        for (column = row + 1U; column < dimension; ++column) {
            value -= lu->data[row * dimension + column] * (*solution)->data[column];
        }
        if (fabs(lu->data[row * dimension + row]) <= lu->threshold) {
            numerix_vector_destroy(*solution);
            *solution = NULL;
            return bad(error, NUMERIX_STATUS_SINGULAR_MATRIX, "singular LU diagonal");
        }
        (*solution)->data[row] = value / lu->data[row * dimension + row];
        if (!isfinite((*solution)->data[row])) {
            numerix_vector_destroy(*solution);
            *solution = NULL;
            return bad(error, NUMERIX_STATUS_NUMERICAL_FAILURE, "non-finite solution");
        }
    }
    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;
}
static numerix_status residual_norm(const numerix_matrix*a,const numerix_vector*x,const numerix_vector*b,double*out,numerix_error*e){size_t i,j;double norm=0;if(!out)return NUMERIX_STATUS_SUCCESS;for(i=0;i<a->rows;i++){double v=-b->data[i];for(j=0;j<a->columns;j++)v+=a->data[i*a->columns+j]*x->data[j];if(fabs(v)>norm)norm=fabs(v);}*out=norm;if(!isfinite(norm))return bad(e,NUMERIX_STATUS_NUMERICAL_FAILURE,"residual overflow");return NUMERIX_STATUS_SUCCESS;}
numerix_status numerix_linear_solve_gaussian(const numerix_matrix*a,const numerix_vector*b,const numerix_options*o,numerix_vector**x,double*res,numerix_error*e){numerix_lu lu;numerix_options opt;numerix_status s;if(!x)return bad(e,NUMERIX_STATUS_INVALID_ARGUMENT,"solution output required");*x=NULL;if(!matok(a)||!vecok(b)||a->rows!=a->columns||b->size!=a->rows)return bad(e,NUMERIX_STATUS_INVALID_DIMENSIONS,"linear system must be square and dimensionally compatible");if(o)opt=*o;else numerix_options_init(&opt);s=numerix_options_validate(&opt,e);if(s)return s;s=lu_raw(a,opt.absolute_tolerance,&lu,e);if(s)return s;s=numerix_lu_solve(&lu,b,x,e);numerix_lu_destroy(&lu);if(!s)s=residual_norm(a,*x,b,res,e);return s;}
numerix_status numerix_linear_solve_gauss_jordan(const numerix_matrix*a,const numerix_vector*b,const numerix_options*o,numerix_vector**x,double*res,numerix_error*e){numerix_matrix*m=NULL;numerix_vector*v=NULL;numerix_options opt;size_t n,i,j,k,p;double maxv=0,threshold;numerix_status s;if(!x)return bad(e,NUMERIX_STATUS_INVALID_ARGUMENT,"solution output required");*x=NULL;if(!matok(a)||!vecok(b)||a->rows!=a->columns||b->size!=a->rows)return bad(e,NUMERIX_STATUS_INVALID_DIMENSIONS,"linear system must be square and dimensionally compatible");if(o)opt=*o;else numerix_options_init(&opt);s=numerix_options_validate(&opt,e);if(s)return s;n=a->rows;s=numerix_matrix_create(n,n,&m,e);if(s)return s;s=numerix_vector_copy(b,&v,e);if(s){numerix_matrix_destroy(m);return s;}memcpy(m->data,a->data,n*n*sizeof(double));for(i=0;i<n*n;i++)if(fabs(a->data[i])>maxv)maxv=fabs(a->data[i]);threshold=fmax(DBL_MIN,DBL_EPSILON*(double)n*maxv);for(k=0;k<n;k++){p=k;for(i=k+1;i<n;i++)if(fabs(m->data[i*n+k])>fabs(m->data[p*n+k]))p=i;if(fabs(m->data[p*n+k])<=threshold){s=bad(e,NUMERIX_STATUS_SINGULAR_MATRIX,"singular pivot in Gauss-Jordan elimination");goto cleanup;}if(p!=k){for(j=0;j<n;j++){double t=m->data[k*n+j];m->data[k*n+j]=m->data[p*n+j];m->data[p*n+j]=t;}{double t=v->data[k];v->data[k]=v->data[p];v->data[p]=t;}}{double pivot=m->data[k*n+k];for(j=0;j<n;j++)m->data[k*n+j]/=pivot;v->data[k]/=pivot;}for(i=0;i<n;i++)if(i!=k){double factor=m->data[i*n+k];for(j=0;j<n;j++)m->data[i*n+j]-=factor*m->data[k*n+j];v->data[i]-=factor*v->data[k];}}if(!vecok(v)){s=bad(e,NUMERIX_STATUS_NUMERICAL_FAILURE,"non-finite solution");goto cleanup;}*x=v;v=NULL;s=residual_norm(a,*x,b,res,e);cleanup:numerix_matrix_destroy(m);numerix_vector_destroy(v);return s;}
