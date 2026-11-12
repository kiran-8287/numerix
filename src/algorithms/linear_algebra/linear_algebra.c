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
    double *row_scale = NULL;
    double relative_threshold;
    numerix_status status = NUMERIX_STATUS_SUCCESS;
    const char *failure_message;

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
    row_scale = malloc(dimension * sizeof(double));
    if (lu->data == NULL || lu->permutation == NULL || row_scale == NULL) {
        free(row_scale);
        numerix_lu_destroy(lu);
        return bad(error, NUMERIX_STATUS_ALLOCATION_FAILURE, "LU allocation failed");
    }
    memcpy(lu->data, matrix->data, dimension * dimension * sizeof(double));
    lu->size = dimension;
    lu->sign = 1;
    relative_threshold = DBL_EPSILON * (double)dimension;
    lu->threshold = relative_threshold;
    for (row = 0; row < dimension; ++row) {
        row_scale[row] = 0.0;
        lu->permutation[row] = row;
        for (column = 0; column < dimension; ++column) {
            double magnitude = fabs(matrix->data[row * dimension + column]);

            if (magnitude > row_scale[row]) {
                row_scale[row] = magnitude;
            }
        }
    }
    for (pivot_column = 0; pivot_column < dimension; ++pivot_column) {
        double best_quality = row_scale[pivot_column] == 0.0 ? 0.0 :
            fabs(lu->data[pivot_column * dimension + pivot_column]) /
                row_scale[pivot_column];

        pivot_row = pivot_column;
        for (row = pivot_column + 1U; row < dimension; ++row) {
            double quality = row_scale[row] == 0.0 ? 0.0 :
                fabs(lu->data[row * dimension + pivot_column]) / row_scale[row];

            if (quality > best_quality) {
                best_quality = quality;
                pivot_row = row;
            }
        }
        if (best_quality <= relative_threshold) {
            status = NUMERIX_STATUS_SINGULAR_MATRIX;
            failure_message = "matrix has a zero or numerically negligible scaled pivot";
            goto cleanup;
        }
        if (pivot_row != pivot_column) {
            size_t permutation;
            double scale;

            for (column = 0; column < dimension; ++column) {
                double element = lu->data[pivot_column * dimension + column];

                lu->data[pivot_column * dimension + column] =
                    lu->data[pivot_row * dimension + column];
                lu->data[pivot_row * dimension + column] = element;
            }
            permutation = lu->permutation[pivot_column];
            lu->permutation[pivot_column] = lu->permutation[pivot_row];
            lu->permutation[pivot_row] = permutation;
            scale = row_scale[pivot_column];
            row_scale[pivot_column] = row_scale[pivot_row];
            row_scale[pivot_row] = scale;
            lu->sign = -lu->sign;
        }
        for (row = pivot_column + 1U; row < dimension; ++row) {
            double multiplier = lu->data[row * dimension + pivot_column] /
                lu->data[pivot_column * dimension + pivot_column];

            if (!isfinite(multiplier)) {
                status = NUMERIX_STATUS_NUMERICAL_FAILURE;
                failure_message = "non-finite LU elimination multiplier";
                goto cleanup;
            }
            lu->data[row * dimension + pivot_column] = multiplier;
            for (column = pivot_column + 1U; column < dimension; ++column) {
                double product = multiplier *
                    lu->data[pivot_column * dimension + column];
                double updated = lu->data[row * dimension + column] - product;

                if (!isfinite(product) || !isfinite(updated)) {
                    status = NUMERIX_STATUS_NUMERICAL_FAILURE;
                    failure_message = "non-finite LU factor during elimination";
                    goto cleanup;
                }
                lu->data[row * dimension + column] = updated;
            }
        }
    }
    free(row_scale);
    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;

cleanup:
    free(row_scale);
    numerix_lu_destroy(lu);
    return bad(error, status, failure_message);
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
        if (lu->data[row * dimension + row] == 0.0) {
            numerix_vector_destroy(*solution);
            *solution = NULL;
            return bad(error, NUMERIX_STATUS_SINGULAR_MATRIX, "singular LU diagonal");
        }
        if (!isfinite(lu->data[row * dimension + row])) {
            numerix_vector_destroy(*solution);
            *solution = NULL;
            return bad(error, NUMERIX_STATUS_NUMERICAL_FAILURE,
                       "non-finite LU diagonal");
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
numerix_status numerix_linear_solve_gauss_jordan(const numerix_matrix *matrix,
                                                 const numerix_vector *right,
                                                 const numerix_options *options,
                                                 numerix_vector **solution,
                                                 double *residual,
                                                 numerix_error *error)
{
    numerix_matrix *work = NULL;
    numerix_vector *values = NULL;
    numerix_options configured;
    double *row_scale = NULL;
    double relative_threshold;
    size_t dimension;
    size_t row;
    size_t column;
    size_t pivot_column;
    numerix_status status;

    if (solution == NULL) {
        return bad(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                   "solution output required");
    }
    *solution = NULL;
    if (!matok(matrix) || !vecok(right) || matrix->rows != matrix->columns ||
        right->size != matrix->rows) {
        return bad(error, NUMERIX_STATUS_INVALID_DIMENSIONS,
                   "linear system must be square and dimensionally compatible");
    }
    if (options != NULL) {
        configured = *options;
    } else {
        numerix_options_init(&configured);
    }
    status = numerix_options_validate(&configured, error);
    if (status != NUMERIX_STATUS_SUCCESS) {
        return status;
    }
    dimension = matrix->rows;
    status = numerix_matrix_create(dimension, dimension, &work, error);
    if (status != NUMERIX_STATUS_SUCCESS) {
        return status;
    }
    status = numerix_vector_copy(right, &values, error);
    if (status != NUMERIX_STATUS_SUCCESS) {
        numerix_matrix_destroy(work);
        return status;
    }
    row_scale = malloc(dimension * sizeof(double));
    if (row_scale == NULL) {
        status = bad(error, NUMERIX_STATUS_ALLOCATION_FAILURE,
                     "Gauss-Jordan row-scale allocation failed");
        goto cleanup;
    }
    memcpy(work->data, matrix->data,
           dimension * dimension * sizeof(double));
    relative_threshold = DBL_EPSILON * (double)dimension;
    for (row = 0; row < dimension; ++row) {
        row_scale[row] = 0.0;
        for (column = 0; column < dimension; ++column) {
            double magnitude = fabs(matrix->data[row * dimension + column]);

            if (magnitude > row_scale[row]) {
                row_scale[row] = magnitude;
            }
        }
    }
    for (pivot_column = 0; pivot_column < dimension; ++pivot_column) {
        size_t pivot_row = pivot_column;
        double best_quality = row_scale[pivot_row] == 0.0 ? 0.0 :
            fabs(work->data[pivot_row * dimension + pivot_column]) /
                row_scale[pivot_row];
        double pivot;

        for (row = pivot_column + 1U; row < dimension; ++row) {
            double quality = row_scale[row] == 0.0 ? 0.0 :
                fabs(work->data[row * dimension + pivot_column]) / row_scale[row];

            if (quality > best_quality) {
                best_quality = quality;
                pivot_row = row;
            }
        }
        if (best_quality <= relative_threshold) {
            status = bad(error, NUMERIX_STATUS_SINGULAR_MATRIX,
                         "singular or numerically negligible scaled pivot in Gauss-Jordan elimination");
            goto cleanup;
        }
        if (pivot_row != pivot_column) {
            double scale;
            double value;

            for (column = 0; column < dimension; ++column) {
                value = work->data[pivot_column * dimension + column];
                work->data[pivot_column * dimension + column] =
                    work->data[pivot_row * dimension + column];
                work->data[pivot_row * dimension + column] = value;
            }
            value = values->data[pivot_column];
            values->data[pivot_column] = values->data[pivot_row];
            values->data[pivot_row] = value;
            scale = row_scale[pivot_column];
            row_scale[pivot_column] = row_scale[pivot_row];
            row_scale[pivot_row] = scale;
        }
        pivot = work->data[pivot_column * dimension + pivot_column];
        if (!isfinite(pivot) || pivot == 0.0) {
            status = bad(error, NUMERIX_STATUS_NUMERICAL_FAILURE,
                         "invalid Gauss-Jordan pivot");
            goto cleanup;
        }
        for (column = 0; column < dimension; ++column) {
            work->data[pivot_column * dimension + column] /= pivot;
            if (!isfinite(work->data[pivot_column * dimension + column])) {
                status = bad(error, NUMERIX_STATUS_NUMERICAL_FAILURE,
                             "non-finite Gauss-Jordan normalized row");
                goto cleanup;
            }
        }
        values->data[pivot_column] /= pivot;
        if (!isfinite(values->data[pivot_column])) {
            status = bad(error, NUMERIX_STATUS_NUMERICAL_FAILURE,
                         "non-finite Gauss-Jordan right-hand side");
            goto cleanup;
        }
        for (row = 0; row < dimension; ++row) {
            double factor;

            if (row == pivot_column) {
                continue;
            }
            factor = work->data[row * dimension + pivot_column];
            for (column = 0; column < dimension; ++column) {
                double updated = work->data[row * dimension + column] -
                    factor * work->data[pivot_column * dimension + column];

                if (!isfinite(updated)) {
                    status = bad(error, NUMERIX_STATUS_NUMERICAL_FAILURE,
                                 "non-finite Gauss-Jordan elimination value");
                    goto cleanup;
                }
                work->data[row * dimension + column] = updated;
            }
            values->data[row] -= factor * values->data[pivot_column];
            if (!isfinite(values->data[row])) {
                status = bad(error, NUMERIX_STATUS_NUMERICAL_FAILURE,
                             "non-finite Gauss-Jordan solution value");
                goto cleanup;
            }
        }
    }
    *solution = values;
    values = NULL;
    status = residual_norm(matrix, *solution, right, residual, error);

cleanup:
    free(row_scale);
    numerix_matrix_destroy(work);
    numerix_vector_destroy(values);
    return status;
}
