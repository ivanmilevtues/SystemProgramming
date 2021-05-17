void swap(int * a, int * b)  {
	*a = *a + *b;
	*b = *a - *b;
	*a = *a - *b;
}


void bubble_sort(int * array, int size) {
	int i, j;
	for (i = 0; i < size; i++) {
		for (j = 0; j < size - i; j++) {
			if(array[i] > array[j]) {
				swap(array + i, array + j);
			}
		}
	}
}


void quick_sort_helper(int * array, int start, int end) {
	int i, j, pivot, temp;
	pivot = start;
	i = start;
	j = end;
	if(start < end) {
		while (i < j) {
			while (array[i] <= array[pivot] && i < end) {
				i++;
			}

			while (array[j] > array[pivot] && j > start) {
				j--;
			}

			if (i < j) {
				swap(array + i, array + j);
			}
		}

		temp = array[pivot];
		array[pivot] = array[j];
		array[j] = temp;

		quick_sort_helper(array, start, j - 1);
		quick_sort_helper(array, j + 1, end);

	}
}


void quick_sort(int  * array, int size) {
	quick_sort_helper(array, 0, size - 1);
}


void selection_sort(int * array, int size) {
	int i, j, min_indx;

	for (i = 0; i < size - 1; i++) {
		min_indx = i;
		for (j = i + 1; j < size; j++) {
			min_indx = j;
		}

		swap(array + min_indx, array + i);
	}
}


void heapify(int * arr, int n, int i ) {
	int largest = i;
	int l = 2 * i + 1;
	int r = 2 * i + 2;

	if (l < n && arr[l] > arr[largest]) {
		largest = l;
	}

	if (r < n && arr[r] > arr[largest]) {
		largest = r;
	}

	if (largest != i) {
		swap(arr + i, arr + largest);

		heapify(arr, n, largest);
	}
}


void heap_sort(int * array, int size) {
	int i;

	for (i = size / 2; i >= 0; i--) {
		heapify(array, size, i);
	}

	for (i = size - 1; i > 0; i--) {
		swap(array, array + i);

		heapify(array, i , 0);
	}
}


void stable_selection_sort(int * array, int size) {
	int i, j, min, key;

	for(i = 0; i < size - 1; i++) {
		min = i;
		
		for(j = i + 1; j < size; j++) {
			if (array[min] > array[j]) {
				min = j;
			}
		}

		key = array[min];
		while(min > i) {
			array[min] = array[min - 1];
			min--;
		}

		array[i] = key;
	}
}