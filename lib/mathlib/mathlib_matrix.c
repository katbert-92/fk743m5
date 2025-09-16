#include "mathlib_matrix.h"
#include "mathlib_wrapper.h"
#include <float.h>

static bool Matrix_IsCorrupted(Matrix_t m) {
	return (m.Data == NULL || m.Rows == 0 || m.Cols == 0);
}

Matrix_t Matrix_Alloc(u32 rows, u32 cols) {
	Matrix_t m;
	m.Rows = (rows == 0) ? 1 : rows;
	m.Cols = (cols == 0) ? 1 : cols;
	m.Data = (double**)MATHLIB_MALLOC(sizeof(double*) * m.Rows);
	if (m.Data == NULL)
		return m;

	for (u32 i = 0; i < m.Rows; ++i) {
		m.Data[i] = (double*)MATHLIB_MALLOC(sizeof(double) * m.Cols);
		if (m.Data[i] == NULL) {
			Matrix_Free(&m);
			break;
		}

		for (u32 j = 0; j < m.Cols; ++j)
			m.Data[i][j] = 0.0;
	}

	return m;
}

void Matrix_Free(Matrix_t* pMatrix) {
	for (u32 i = 0; i < pMatrix->Rows; ++i)
		MATHLIB_FREE(pMatrix->Data[i]);

	MATHLIB_FREE(pMatrix->Data);

	pMatrix->Data = NULL;
	pMatrix->Rows = 0;
	pMatrix->Cols = 0;
}

RET_STATE_t Matrix_IdentityMatrix_Set(Matrix_t m) {
	if (Matrix_IsCorrupted(m))
		return RET_STATE_ERR_PARAM;

	for (u32 i = 0; i < m.Rows; ++i) {
		for (u32 j = 0; j < m.Cols; ++j) {
			if (i == j)
				m.Data[i][j] = 1.0;
			else
				m.Data[i][j] = 0.0;
		}
	}

	return RET_STATE_SUCCESS;
}

RET_STATE_t Matrix_Copy(Matrix_t destination, Matrix_t source) {
	if (Matrix_IsCorrupted(destination) || Matrix_IsCorrupted(source) ||
		destination.Rows < source.Rows || destination.Cols < source.Cols) {
		return RET_STATE_ERR_PARAM;
	}

	for (u32 i = 0; i < source.Rows; ++i) {
		for (u32 j = 0; j < source.Cols; ++j)
			destination.Data[i][j] = source.Data[i][j];
	}

	return RET_STATE_SUCCESS;
}

RET_STATE_t Matrix_Add(Matrix_t a, Matrix_t b, Matrix_t resultMatrix) {
	if (Matrix_IsCorrupted(a) || Matrix_IsCorrupted(b) || Matrix_IsCorrupted(resultMatrix) ||
		a.Rows != b.Rows || a.Rows != resultMatrix.Rows || a.Cols != b.Cols ||
		a.Cols != resultMatrix.Cols) {
		return RET_STATE_ERR_PARAM;
	}

	for (u32 i = 0; i < a.Rows; ++i) {
		for (u32 j = 0; j < a.Cols; ++j)
			resultMatrix.Data[i][j] = a.Data[i][j] + b.Data[i][j];
	}

	return RET_STATE_SUCCESS;
}

RET_STATE_t Matrix_Subtract(Matrix_t a, Matrix_t b, Matrix_t resultMatrix) {
	if (Matrix_IsCorrupted(a) || Matrix_IsCorrupted(b) || Matrix_IsCorrupted(resultMatrix) ||
		a.Rows != b.Rows || a.Rows != resultMatrix.Rows || a.Cols != b.Cols ||
		a.Cols != resultMatrix.Cols) {
		return RET_STATE_ERR_PARAM;
	}

	for (u32 i = 0; i < a.Rows; ++i) {
		for (u32 j = 0; j < a.Cols; ++j)
			resultMatrix.Data[i][j] = a.Data[i][j] - b.Data[i][j];
	}

	return RET_STATE_SUCCESS;
}

RET_STATE_t Matrix_SubtractFromIdentity(Matrix_t a) {
	if (Matrix_IsCorrupted(a))
		return RET_STATE_ERR_PARAM;

	for (u32 i = 0; i < a.Rows; ++i) {
		for (u32 j = 0; j < a.Cols; ++j) {
			if (i == j)
				a.Data[i][j] = 1.0 - a.Data[i][j];
			else
				a.Data[i][j] = 0.0 - a.Data[i][j];
		}
	}

	return RET_STATE_SUCCESS;
}

RET_STATE_t Matrix_Multiply(Matrix_t a, Matrix_t b, Matrix_t resultMatrix) {
	if (Matrix_IsCorrupted(a) || Matrix_IsCorrupted(b) || Matrix_IsCorrupted(resultMatrix) ||
		a.Cols != b.Rows || a.Rows != resultMatrix.Rows || b.Cols != resultMatrix.Cols) {
		return RET_STATE_ERR_PARAM;
	}

	for (u32 i = 0; i < resultMatrix.Rows; ++i) {
		for (u32 j = 0; j < resultMatrix.Cols; ++j) {
			resultMatrix.Data[i][j] = 0.0;
			for (u32 k = 0; k < a.Cols; ++k)
				resultMatrix.Data[i][j] += a.Data[i][k] * b.Data[k][j];
		}
	}

	return RET_STATE_SUCCESS;
}

RET_STATE_t Matrix_MultiplyByTranspose(Matrix_t a, Matrix_t b, Matrix_t resultMatrix) {
	if (Matrix_IsCorrupted(a) || Matrix_IsCorrupted(b) || Matrix_IsCorrupted(resultMatrix) ||
		a.Cols != b.Cols || a.Rows != resultMatrix.Rows || b.Rows != resultMatrix.Cols) {
		return RET_STATE_ERR_PARAM;
	}

	for (u32 i = 0; i < resultMatrix.Rows; ++i) {
		for (u32 j = 0; j < resultMatrix.Cols; ++j) {
			/* Calculate element c.data[i][j] via a dot product of one row of a
			 	 with one row of b */
			resultMatrix.Data[i][j] = 0.0;
			for (u32 k = 0; k < a.Cols; ++k)
				resultMatrix.Data[i][j] += a.Data[i][k] * b.Data[j][k];
		}
	}

	return RET_STATE_SUCCESS;
}

RET_STATE_t Matrix_Transpose(Matrix_t input, Matrix_t output) {
	if (Matrix_IsCorrupted(input) || Matrix_IsCorrupted(output) || input.Cols != output.Rows ||
		input.Rows != output.Cols) {
		return RET_STATE_ERR_PARAM;
	}

	for (u32 i = 0; i < input.Rows; ++i) {
		for (u32 j = 0; j < input.Cols; ++j)
			output.Data[j][i] = input.Data[i][j];
	}

	return RET_STATE_SUCCESS;
}

bool Matrix_IsEqual(Matrix_t a, Matrix_t b, double tolerance) {
	if (Matrix_IsCorrupted(a) || Matrix_IsCorrupted(b) || a.Rows != b.Rows || a.Cols != b.Cols) {
		return false;
	}

	for (u32 i = 0; i < a.Rows; ++i) {
		for (u32 j = 0; j < a.Cols; ++j) {
			if (abs(a.Data[i][j] - b.Data[i][j]) > tolerance)
				return false;
		}
	}

	return true;
}

RET_STATE_t Matrix_Scale(Matrix_t m, double scalar) {
	if (Matrix_IsCorrupted(m))
		return RET_STATE_ERR_PARAM;

	for (u32 i = 0; i < m.Rows; ++i)
		for (u32 j = 0; j < m.Cols; ++j)
			m.Data[i][j] *= scalar;

	return RET_STATE_SUCCESS;
}

RET_STATE_t Matrix_Rows_Swap(Matrix_t m, u32 row1, u32 row2) {
	if (Matrix_IsCorrupted(m) || row1 > m.Rows || row2 > m.Rows)
		return RET_STATE_ERR_PARAM;

	double* tmp	 = m.Data[row1];
	m.Data[row1] = m.Data[row2];
	m.Data[row2] = tmp;

	return RET_STATE_SUCCESS;
}

RET_STATE_t Matrix_Row_Scale(Matrix_t m, u32 row, double scalar) {
	if (Matrix_IsCorrupted(m) || row > m.Rows)
		return RET_STATE_ERR_PARAM;

	for (u32 i = 0; i < m.Cols; ++i)
		m.Data[row][i] *= scalar;

	return RET_STATE_SUCCESS;
}

RET_STATE_t Matrix_Row_Shear(Matrix_t m, u32 row1, u32 row2, double scalar) {
	if (Matrix_IsCorrupted(m) || row1 > m.Rows || row2 > m.Rows)
		return RET_STATE_ERR_PARAM;

	for (u32 i = 0; i < m.Cols; ++i)
		m.Data[row1][i] += scalar * m.Data[row2][i];

	return RET_STATE_SUCCESS;
}

/* 
 * Uses Gauss-Jordan elimination.
 * The elimination procedure works by applying elementary row
 * operations to our input matrix until the input matrix is reduced to
 * the identity matrix.
 * Simultaneously, we apply the same elementary row operations to a
 * separate identity matrix to produce the inverse matrix.
 * If this makes no sense, read wikipedia on Gauss-Jordan elimination 
 * This is not the fastest way to invert matrices, so this is quite
 * possibly the bottleneck.
*/
RET_STATE_t Matrix_Destructive_Invert(Matrix_t input, Matrix_t output) {
	if (Matrix_IsCorrupted(input) || Matrix_IsCorrupted(output) || input.Rows != input.Cols ||
		input.Rows != output.Rows || input.Rows != output.Cols) {
		return RET_STATE_ERR_PARAM;
	}

	Matrix_IdentityMatrix_Set(output);

	/* Convert input to the identity matrix via elementary row operations.
		 The ith pass through this loop turns the element at i,i to a 1
		 and turns all other elements in column i to a 0. */
	for (u32 i = 0; i < input.Rows; ++i) {
		if (input.Data[i][i] == 0.0) {
			/* We must swap rows to get a nonzero diagonal element. */
			u32 r;
			for (r = i + 1; r < input.Rows; ++r)
				if (input.Data[r][i] != 0.0)
					break;

			if (r == input.Rows) {
				/* Every remaining element in this column is zero, so this
					matrix cannot be inverted. */
				return RET_STATE_ERR_EMPTY;
			}

			Matrix_Rows_Swap(input, i, r);
			Matrix_Rows_Swap(output, i, r);
		}

		/* Scale this row to ensure a 1 along the diagonal.
			 We might need to worry about overflow from a huge scalar here. */
		double scalar = 1.0 / input.Data[i][i];
		if (scalar >= DBL_MAX || scalar <= -DBL_MAX)
			return RET_STATE_ERR_OVERFLOW;

		Matrix_Row_Scale(input, i, scalar);
		Matrix_Row_Scale(output, i, scalar);

		/* Zero out the other elements in this column. */
		for (u32 j = 0; j < input.Rows; ++j) {
			if (i == j)
				continue;

			double shear_needed = -input.Data[j][i];
			Matrix_Row_Shear(input, j, i, shear_needed);
			Matrix_Row_Shear(output, j, i, shear_needed);
		}
	}

	return RET_STATE_SUCCESS;
}
