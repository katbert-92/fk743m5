#ifndef __MATHLIB_MATRIX_H
#define __MATHLIB_MATRIX_H

#include "main.h"

/**
 * @brief Matrix structure
 * 
 */
typedef struct {
	u32 Rows;	   /*!< Amount of rows */
	u32 Cols;	   /*!< Amount of columns */
	double** Data; /*!< Pointer to two-dimensional array with matrix data */
} Matrix_t;

/**
 * @brief Function that dynamicly allocates memory for matrix.
 * 
 * @param[in] rows: Amount of rows in matrix.
 * @param[in] cols: Amount of columns in matrix.
 * 
 * @return A rows-by-cols matrix filled with zeros.
 */
Matrix_t Matrix_Alloc(u32 rows, u32 cols);

/**
 * @brief Function that releases memory allocated for matrix.
 * 
 * @param[in] pMatrix: Pointer to input matrix.
 */
void Matrix_Free(Matrix_t* pMatrix);

/**
 * @brief Turn matrix into an identity matrix.
 * 
 * @param[in] m: Input matrix.
 * 
 * @retval RET_STATE_SUCCES: Successfull.
 * @retval RET_STATE_ERR_PARAM: Something wrong with input parameters.
 */
RET_STATE_t Matrix_IdentityMatrix_Set(Matrix_t m);

/**
 * @brief Copy matrix data.
 * 
 * @param[in] destination: Matrix to copy to.
 * @param[in] source: Matrix to copy from.
 * 
 * @retval RET_STATE_SUCCES: Successfull.
 * @retval RET_STATE_ERR_PARAM: Something wrong with input parameters.
 */
RET_STATE_t Matrix_Copy(Matrix_t destination, Matrix_t source);

/**
 * @brief Add matrices a and b and put the result in resultMatrix.
 * 
 * @param[in] a: First matrix summand.
 * @param[in] b: Second matrix summand.
 * @param[out] resultMatrix: Sum of matrices a and b.
 * 
 * @retval RET_STATE_SUCCES: Successfull.
 * @retval RET_STATE_ERR_PARAM: Something wrong with input parameters.
 */
RET_STATE_t Matrix_Add(Matrix_t a, Matrix_t b, Matrix_t resultMatrix);

/**
 * @brief Subtract matrices a and b and put the result in resultMatrix.
 * 
 * @param[in] a: Matrix to substract from.
 * @param[in] b: Matrix which will be substracted.
 * @param[out] resultMatrix: Result of a and b matrices substraction.
 * 
 * @retval RET_STATE_SUCCES: Successfull.
 * @retval RET_STATE_ERR_PARAM: Something wrong with input parameters.
 */
RET_STATE_t Matrix_Subtract(Matrix_t a, Matrix_t b, Matrix_t resultMatrix);

/**
 * @brief Subtract from the identity matrix in place.
 * 
 * @param[in, out] a: Matrix that will be substracted from identity matrix.
 * 
 * @retval RET_STATE_SUCCES: Successfull.
 * @retval RET_STATE_ERR_PARAM: Something wrong with input parameters.
 */
RET_STATE_t Matrix_SubtractFromIdentity(Matrix_t a);

/**
 * @brief Multiply matrices a and b and put the result in resultMatrix.
 * 
 * @param[in] a: First matrix multiplier.
 * @param[in] b: Second matrix multiplier.
 * @param[out] resultMatrix: Result of a and b matrix multiplication.
 * 
 * @retval RET_STATE_SUCCES: Successfull.
 * @retval RET_STATE_ERR_PARAM: Something wrong with input parameters.
 */
RET_STATE_t Matrix_Multiply(Matrix_t a, Matrix_t b, Matrix_t resultMatrix);

/**
 * @brief Multiply matrix a by b-transpose and put the result in resultMatrix.
 * @note This is multiplying a by b-tranpose so it is like multiply_matrix
 * but references to b reverse rows and cols.
 * 
 * @param[in] a: First matrix multiplier.
 * @param[in] b: Transposed second matrix multiplier.
 * @param[out] resultMatrix: Result of a and b-transpose matrix multiplication.
 * 
 * @retval RET_STATE_SUCCES: Successfull.
 * @retval RET_STATE_ERR_PARAM: Something wrong with input parameters.
 */
RET_STATE_t Matrix_MultiplyByTranspose(Matrix_t a, Matrix_t b, Matrix_t resultMatrix);

/**
 * @brief Transpose input and put the result in output.
 * 
 * @param[in] input: Matrix that will be transposed.
 * @param[out] output: Result of input matrix transpose.
 * 
 * @retval RET_STATE_SUCCES: Successfull.
 * @retval RET_STATE_ERR_PARAM: Something wrong with input parameters.
 */
RET_STATE_t Matrix_Transpose(Matrix_t input, Matrix_t output);

/**
 * @brief Whether two matrices are approximately equal.
 * 
 * @param[in] a: First matrix to compare.
 * @param[in] b: Second matrix to compare.
 * @param[in] tolerance: Acceptable comparasion tolerance.
 * 
 * @retval true: Input matrices are approximately equal.
 * @retval false: Intput matrices are not equal.
 */
bool Matrix_IsEqual(Matrix_t a, Matrix_t b, double tolerance);

/**
 * @brief Multiply a matrix by a scalar.
 * 
 * @param[in, out] m: Input matrix.
 * @param[in] scalar: Scalar value for multiplication.
 * 
 * @retval RET_STATE_SUCCES: Successfull.
 * @retval RET_STATE_ERR_PARAM: Something wrong with input parameters.
 */
RET_STATE_t Matrix_Scale(Matrix_t m, double scalar);

/**
 * @brief Swap rows row1 and row2 of a matrix.
 * This is one of the three "elementary row operations". 
 * 
 * @param[in, out] m: Input matrix.
 * @param[in] row1: First row to swap.
 * @param[in] row2: Second row to swap.
 * 
 * @retval RET_STATE_SUCCES: Successfull.
 * @retval RET_STATE_ERR_PARAM: Something wrong with input parameters.
 */
RET_STATE_t Matrix_Rows_Swap(Matrix_t m, u32 row1, u32 row2);

/**
 * @brief Multiply row of a matrix by a scalar.
 * This is one of the three "elementary row operations".
 * 
 * @param[in, out] m: Input matrix.
 * @param[in] row: Row that will be multiplied.
 * @param[in] scalar: Scalar value for multiplication.
 * 
 * @retval RET_STATE_SUCCES: Successfull.
 * @retval RET_STATE_ERR_PARAM: Something wrong with input parameters.
 */
RET_STATE_t Matrix_Row_Scale(Matrix_t m, u32 row, double scalar);

/**
 * @brief Add a multiple of row2 to row1.
 * Also known as a "shear" operation.
 * This is one of the three "elementary row operations".
 * 
 * @param[in, out] m: Input matrix.
 * @param[in] row1: Row into which will be added another row.
 * @param[in] row2: Row that will be multiplied by scalar.
 * @param[in] scalar: Scalar value for multiplication.
 * 
 * @retval RET_STATE_SUCCES: Successfull.
 * @retval RET_STATE_ERR_PARAM: Something wrong with input parameters.
 */
RET_STATE_t Matrix_Row_Shear(Matrix_t m, u32 row1, u32 row2, double scalar);

/**
 * @brief Invert a square matrix.
 * input is mutated as well by this routine. 
 * 
 * @param[in] input: Input matrix.
 * @param[out] output: Output matrix.
 * 
 * @retval RET_STATE_SUCCES: Successfull.
 * @retval RET_STATE_ERR_EMPTY: Matrix cannot be inverted.
 * @retval RET_STATE_ERR_OVERFLOW: Scalar is out of range.
 */
RET_STATE_t Matrix_Destructive_Invert(Matrix_t input, Matrix_t output);

#endif /* __MATHLIB_MATRIX_H */
