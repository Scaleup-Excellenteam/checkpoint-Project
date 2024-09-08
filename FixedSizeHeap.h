#ifndef CHECKPOINT_FIXEDSIZEHEAP_H
#define CHECKPOINT_FIXEDSIZEHEAP_H

#define MaxSize 10
#include "student_management_system.h"

typedef struct FixedSizeMaxHeap
{
    int Grade_Level;
    int CourseNumber;
    Student students[MaxSize];
    int FilledIndex;      // Tracks the number of entries
    int overall_grade;    // Sum of all grades
    int studentsCount;    // Number of students
} FixedSizeMaxHeap;

// Declaration of heapMatrix using extern
extern FixedSizeMaxHeap* heapMatrix[12][10];

FixedSizeMaxHeap *CreateMaxHeap(int Grade_Level , int CourseNumber);
void SiftUp(FixedSizeMaxHeap *maxHeap);
void SiftDown(FixedSizeMaxHeap *maxHeap);
void insert(FixedSizeMaxHeap *maxHeap, Student *student);
void Delete(FixedSizeMaxHeap *maxHeap, Student *Student);
void update(FixedSizeMaxHeap *maxHeap, Student *NewStudent);
void printHeap(FixedSizeMaxHeap *maxHeap);
void CreateHeapMatrix();

// Functions for JSON I/O
void SaveHeapToJson(FixedSizeMaxHeap *heap, const char *filename);
FixedSizeMaxHeap *LoadHeapFromJson(const char *filename);
void FreeHeap(FixedSizeMaxHeap *heap);

#endif //CHECKPOINT_FIXEDSIZEHEAP_H
