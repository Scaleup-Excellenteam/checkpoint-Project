#include <stdbool.h>
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "FixedSizeHeap.h"
#include "cJSON.h"

FixedSizeMaxHeap* heapMatrix[12][10];

FixedSizeMaxHeap *CreateMaxHeap(int Grade_Level, int CourseNumber)
{
    // Allocate memory for the heap structure
    FixedSizeMaxHeap *MaxHeap = calloc(1, sizeof(FixedSizeMaxHeap));
    if (!MaxHeap)
    {
        perror("Failed to create heap");
        return NULL;
    }

    MaxHeap->Grade_Level = Grade_Level;
    MaxHeap->CourseNumber = CourseNumber;
    MaxHeap->FilledIndex = 0;
    MaxHeap->studentsCount = 0;
    MaxHeap->overall_grade = 0;
    return MaxHeap;
}

void insert(FixedSizeMaxHeap *maxHeap, Student *student)
{
    int index = maxHeap->FilledIndex;
    if (index < MaxSize)
    {
        maxHeap->students[index] = *student;
        if (maxHeap->FilledIndex < MaxSize)
            maxHeap->FilledIndex++;
    }

    bool flag = false;
    // Check if there is a lower grade to replace
    for (int i = 0; i < MaxSize; i++)
    {
        if (maxHeap->students[i].grades[maxHeap->CourseNumber] < student->grades[maxHeap->CourseNumber])
        {
            flag = true;
            index = i;
            break;
        }
    }

    if (flag)
    {
        maxHeap->students[index] = *student;
        maxHeap->overall_grade += student->grades[maxHeap->CourseNumber];
    } else
    {
        maxHeap->overall_grade += student->grades[maxHeap->CourseNumber];
    }

    maxHeap->studentsCount++;
    SiftUp(maxHeap);
    SiftDown(maxHeap);
}

void Delete(FixedSizeMaxHeap *maxHeap, Student *Student)
{
    for (int i = 0; i < MaxSize; i++)
    {
        if (strcmp(maxHeap->students[i].first_name, Student->first_name) == 0 && strcmp(maxHeap->students[i].last_name, Student->last_name) == 0)
        {
            maxHeap->students[i] = *Student;
        }
    }
    maxHeap->overall_grade -= Student->grades[maxHeap->CourseNumber];
    maxHeap->studentsCount--;
}

void update(FixedSizeMaxHeap *maxHeap, Student *NewStudent)
{
    for (int i = 0; i < MaxSize; i++)
    {
        if (strcmp(maxHeap->students[i].first_name, NewStudent->first_name) == 0 && strcmp(NewStudent->last_name, maxHeap->students[i].last_name) == 0)
        {
            if (maxHeap->students[i].grades[maxHeap->CourseNumber] > NewStudent->grades[maxHeap->CourseNumber])
                maxHeap->overall_grade -= (maxHeap->students[i].grades[maxHeap->CourseNumber] - NewStudent->grades[maxHeap->CourseNumber]);
            else
                maxHeap->overall_grade += (NewStudent->grades[maxHeap->CourseNumber] - maxHeap->students[i].grades[maxHeap->CourseNumber]);

            maxHeap->students[i] = *NewStudent;
        }
    }

}

void SiftUp(FixedSizeMaxHeap *maxHeap)
{
    int index = maxHeap->FilledIndex - 1;

    while (index > 0)
    {
        int parentIndex = (index - 1) / 2;

        if (maxHeap->students[index].grades[maxHeap->CourseNumber] > maxHeap->students[parentIndex].grades[maxHeap->CourseNumber])
        {
            Student temp = maxHeap->students[index];
            maxHeap->students[index] = maxHeap->students[parentIndex];
            maxHeap->students[parentIndex] = temp;

            index = parentIndex;
        }
        else
        {
            break;
        }
    }
}

void SiftDown(FixedSizeMaxHeap *maxHeap)
{
    int index = 0;

    while (true)
    {
        int leftChildIndex = 2 * index + 1;
        int rightChildIndex = 2 * index + 2;
        int largest = index;

        if (leftChildIndex < maxHeap->FilledIndex && maxHeap->students[leftChildIndex].grades[maxHeap->CourseNumber] > maxHeap->students[largest].grades[maxHeap->CourseNumber])
        {
            largest = leftChildIndex;
        }

        if (rightChildIndex < maxHeap->FilledIndex && maxHeap->students[rightChildIndex].grades[maxHeap->CourseNumber] > maxHeap->students[largest].grades[maxHeap->CourseNumber])
        {
            largest = rightChildIndex;
        }

        if (largest == index)
        {
            break;
        }

        Student temp = maxHeap->students[index];
        maxHeap->students[index] = maxHeap->students[largest];
        maxHeap->students[largest] = temp;

        index = largest;
    }
}

void printHeap(FixedSizeMaxHeap *maxHeap)
{
    if (maxHeap == NULL) {
        printf("Heap is not initialized.\n");
        return;
    }

    printf("Heap Information:\n");
    printf("Grade Level: %d\n", maxHeap->Grade_Level);
    printf("Course Number: %d\n", maxHeap->CourseNumber);
    printf("Overall Grade: %d\n", maxHeap->overall_grade);
    printf("Number of Students: %d\n", maxHeap->studentsCount);
    printf("Student Information (MaxSize = %d):\n", MaxSize);

    for (int i = 0; i < maxHeap->FilledIndex; i++)
    {
        Student *student = &maxHeap->students[i];
        printf("Student #%d:\n", i + 1);
        printf("  First Name: %s\n", student->first_name);
        printf("  Last Name: %s\n", student->last_name);
        printf("  Phone: %s\n", student->phone);
        printf("  Grade: %d\n", student->grade);
        printf("  Class: %d\n", student->class);
        printf("  Course Grade: %d\n", student->grades[maxHeap->CourseNumber]);
    }
}

void CreateHeapMatrix() {
    for (int grade = 0; grade < 12; grade++) {
        for (int course = 0; course < 10; course++) {
            heapMatrix[grade][course] = CreateMaxHeap(grade, course);
        }
    }
}

// Free heap memory
void FreeHeap(FixedSizeMaxHeap *heap)
{
    free(heap);
}


// Save the entire heap matrix to JSON
bool file_exists(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file != NULL)
    {
        fclose(file);
        return true;
    }
    return false;
}

void SaveHeapMatrixToJson(const char *filename)
{
    // Check if the file already exists
    if (file_exists(filename))
    {
        printf("File '%s' already exists. Skipping save.\n", filename);
        return;
    }

    cJSON *jsonMatrix = cJSON_CreateArray();

    for (int grade = 0; grade < 12; grade++)
    {
        for (int course = 0; course < 10; course++)
        {
            FixedSizeMaxHeap *heap = heapMatrix[grade][course];
            if (heap != NULL)
            {
                cJSON *jsonHeap = cJSON_CreateObject();
                cJSON_AddNumberToObject(jsonHeap, "Grade_Level", heap->Grade_Level);
                cJSON_AddNumberToObject(jsonHeap, "CourseNumber", heap->CourseNumber);
                cJSON_AddNumberToObject(jsonHeap, "overall_grade", heap->overall_grade);
                cJSON_AddNumberToObject(jsonHeap, "studentsCount", heap->studentsCount);

                cJSON *jsonStudents = cJSON_CreateArray();
                for (int i = 0; i < heap->FilledIndex; i++)
                {
                    cJSON *jsonStudent = cJSON_CreateObject();
                    cJSON_AddStringToObject(jsonStudent, "first_name", heap->students[i].first_name);
                    cJSON_AddStringToObject(jsonStudent, "last_name", heap->students[i].last_name);
                    cJSON_AddStringToObject(jsonStudent, "phone", heap->students[i].phone);
                    cJSON_AddNumberToObject(jsonStudent, "grade", heap->students[i].grade);
                    cJSON_AddNumberToObject(jsonStudent, "class", heap->students[i].class);

                    cJSON *gradesArray = cJSON_CreateIntArray(heap->students[i].grades, 10);
                    cJSON_AddItemToObject(jsonStudent, "grades", gradesArray);

                    cJSON_AddItemToArray(jsonStudents, jsonStudent);
                }
                cJSON_AddItemToObject(jsonHeap, "students", jsonStudents);
                cJSON_AddItemToArray(jsonMatrix, jsonHeap);
            }
        }
    }

    FILE *file = fopen(filename, "w");
    if (file)
    {
        char *jsonString = cJSON_Print(jsonMatrix);
        fprintf(file, "%s", jsonString);
        fclose(file);
        free(jsonString);
    }

    cJSON_Delete(jsonMatrix);
}


// Load the heap matrix from JSON
void LoadHeapMatrixFromJson(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Failed to open file");
        return;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *jsonString = (char *)malloc(fileSize + 1);
    fread(jsonString, 1, fileSize, file);
    fclose(file);
    jsonString[fileSize] = '\0';

    cJSON *jsonMatrix = cJSON_Parse(jsonString);
    free(jsonString);

    if (!jsonMatrix)
    {
        return;
    }

    cJSON *jsonHeap;
    cJSON_ArrayForEach(jsonHeap, jsonMatrix)
    {
        int grade = cJSON_GetObjectItem(jsonHeap, "Grade_Level")->valueint;
        int course = cJSON_GetObjectItem(jsonHeap, "CourseNumber")->valueint;

        if (heapMatrix[grade][course] != NULL) {
            FreeHeap(heapMatrix[grade][course]);
        }

        FixedSizeMaxHeap *heap = CreateMaxHeap(grade, course);
        if (!heap) {
            continue;
        }

        heap->overall_grade = cJSON_GetObjectItem(jsonHeap, "overall_grade")->valueint;
        heap->studentsCount = cJSON_GetObjectItem(jsonHeap, "studentsCount")->valueint;

        cJSON *jsonStudents = cJSON_GetObjectItem(jsonHeap, "students");
        if (!jsonStudents || heap->studentsCount == 0) {
            continue;
        }

        int i = 0;
        cJSON *jsonStudent;
        cJSON_ArrayForEach(jsonStudent, jsonStudents)
        {
            strcpy(heap->students[i].first_name, cJSON_GetObjectItem(jsonStudent, "first_name")->valuestring);
            strcpy(heap->students[i].last_name, cJSON_GetObjectItem(jsonStudent, "last_name")->valuestring);
            strcpy(heap->students[i].phone, cJSON_GetObjectItem(jsonStudent, "phone")->valuestring);
            heap->students[i].grade = cJSON_GetObjectItem(jsonStudent, "grade")->valueint;
            heap->students[i].class = cJSON_GetObjectItem(jsonStudent, "class")->valueint;

            cJSON *gradesArray = cJSON_GetObjectItem(jsonStudent, "grades");
            if (gradesArray) {
                for (int j = 0; j < 10; j++) {
                    heap->students[i].grades[j] = cJSON_GetArrayItem(gradesArray, j)->valueint;
                }
            }

            i++;
        }

        heap->FilledIndex = i;
        heapMatrix[grade][course] = heap;
    }

    cJSON_Delete(jsonMatrix);
}


void FreeHeapMatrix() {
    for (int grade = 0; grade < 12; grade++) {
        for (int course = 0; course < 10; course++) {
            if (heapMatrix[grade][course] != NULL) {
                FreeHeap(heapMatrix[grade][course]);
                heapMatrix[grade][course] = NULL;
            }

        }
    }
}
