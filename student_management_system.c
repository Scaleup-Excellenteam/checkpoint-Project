#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "student_management_system.h"
#include "FixedSizeHeap.h"


//structs



//implementation
unsigned long hash(const char* first_name, const char* last_name) {
    unsigned long hash = 5381;
    int c;

    while ((c = *first_name++))
        hash = ((hash << 5) + hash) + tolower(c);

    while ((c = *last_name++))
        hash = ((hash << 5) + hash) + tolower(c);

    return hash % HASH_SIZE;
}

void insert_student(School* school, Student* student) {
    if (!school || !student) return;

    int grade_index = student->grade - 1;
    int class_index = student->class - 1;

    if (grade_index < 0 || grade_index >= MAX_GRADES || class_index < 0 || class_index >= MAX_CLASSES) {
        printf("Invalid grade or class for student %s %s\n", student->first_name, student->last_name);
        return;
    }

    Class* class = &school->grades[grade_index].classes[class_index];

    // Check if we need to resize the students array
    if (class->num_students >= class->capacity) {
        int new_capacity = class->capacity * 2;
        Student** new_students = realloc(class->students, new_capacity * sizeof(Student*));
        if (!new_students) {
            printf("Failed to allocate memory for new student\n");
            return;
        }
        class->students = new_students;
        class->capacity = new_capacity;
    }

    // Add the student to the class
    class->students[class->num_students++] = student;
    school->total_students++;

    // Insert into hash table
    unsigned long index = hash(student->first_name, student->last_name);
    student->next = school->hash_table.buckets[index];
    school->hash_table.buckets[index] = student;
    school->hash_table.size++;
}

School* create_school() {
    School* school = malloc(sizeof(School));
    if (!school) return NULL;

    memset(school, 0, sizeof(School));

    // Initialize grades and classes
    for (int i = 0; i < MAX_GRADES; i++) {
        school->grades[i].grade_id = i + 1;
        for (int j = 0; j < MAX_CLASSES; j++) {
            school->grades[i].classes[j].class_id = j + 1;
            school->grades[i].classes[j].capacity = 10;
            school->grades[i].classes[j].students = malloc(10 * sizeof(Student*));
            if (!school->grades[i].classes[j].students) {
                // Handle allocation failure
                // For simplicity, we're not handling this case fully here
                return NULL;
            }
        }
        school->grades[i].num_classes = MAX_CLASSES;
    }
    school->num_of_grades = MAX_GRADES;

    return school;
}

School* read_data_from_file(const char* file_name) {
    FILE* file = fopen(file_name, "r");
    if (!file) {
        printf("Error opening file.\n");
        return NULL;
    }
    CreateHeapMatrix();
    School* school = create_school();
    if (!school) {
        printf("Failed to create school.\n");
        fclose(file);
        return NULL;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        Student* student = malloc(sizeof(Student));
        if (!student) {
            printf("Memory allocation failed for student.\n");
            continue;
        }

        if (sscanf(line, "%s %s %s %d %d %d %d %d %d %d %d %d %d %d %d",
                   student->first_name, student->last_name, student->phone,
                   &student->grade, &student->class,
                   &student->grades[0], &student->grades[1], &student->grades[2], &student->grades[3],
                   &student->grades[4], &student->grades[5], &student->grades[6], &student->grades[7],
                   &student->grades[8], &student->grades[9]) == 15) {

            int sum = 0;
            for (int i = 0; i < SUBJECTS; i++) {
                sum += student->grades[i];
            }
            student->average_grade = (double)sum / SUBJECTS;

            insert_student(school, student);
            for (int i = 0; i < 10; i++) {
                // Check if heap exists, if not create it
                if (heapMatrix[student->grade][i] == NULL) {
                    heapMatrix[student->grade][i] = CreateMaxHeap(student->grade, i);
                    if (heapMatrix[student->grade][i] == NULL) {
                        printf("Failed to create heap for grade %d, course %d\n", student->grade, i);
                        continue;
                    }
                }
                // Insert the student into the corresponding heap for the course
                insert(heapMatrix[student->grade][i], student);
            }
        } else {
            printf("Error parsing line: %s", line);
            free(student);
        }
    }

    fclose(file);
    printf("Total students added: %d\n", school->total_students);
    return school;
}

void insertNewStudent(School* school) {
    printf("Insert new student function not implemented yet.\n");
}

void deleteStudent(School* school) {
    printf("Delete student function not implemented yet.\n");
}

void editStudentGrade(School* school) {
    printf("Edit student grade function not implemented yet.\n");
}

Student* find(School* school, const char* first_name, const char* last_name) {
    if (!school || !first_name || !last_name) return NULL;

    unsigned long index = hash(first_name, last_name);
    Student* current = school->hash_table.buckets[index];

    while (current != NULL) {
        if (strcasecmp(current->first_name, first_name) == 0 &&
            strcasecmp(current->last_name, last_name) == 0) {
            return current;
        }
        current = current->next;
    }

    return NULL;
}

void searchStudent(School* school) {
    char input[MAX_NAME * 2];
    char first_name[MAX_NAME];
    char last_name[MAX_NAME];

    printf("Please enter name and last name separated by space:\n");
    while (getchar() != '\n');

    if (fgets(input, sizeof(input), stdin) == NULL) {
        printf("Error reading input.\n");
        return;
    }

    input[strcspn(input, "\n")] = 0;

    if (sscanf(input, "%s %s", first_name, last_name) != 2) {
        printf("Invalid input. Please enter both first and last name.\n");
        return;
    }

    Student* student = find(school, first_name, last_name);

    if (student != NULL) {
        printf("Student found:\n");
        printf("Name: %s %s\n", student->first_name, student->last_name);
        printf("Phone: %s\n", student->phone);
        printf("Grade: %d\n", student->grade);
        printf("Class: %d\n", student->class);
        printf("Grades: ");
        for (int i = 0; i < 10; i++) {
            printf("%d ", student->grades[i]);
        }
        printf("\nAverage Grade: %.2f\n", student->average_grade);
    } else {
        printf("Student not found. Please check the spelling and try again.\n");
    }

}

void printAllStudents(School* school) {
    if (school == NULL || school->total_students == 0) {
        printf("No students in the school.\n");
        return;
    }

    printf("\n%-20s %-20s %-15s %-6s %-6s %-30s %-15s\n",
           "First Name", "Last Name", "Phone", "Grade", "Class", "Grades", "Average Grade");
    printf("----------------------------------------------------------------------------------------------------\n");

    for (int i = 0; i < HASH_SIZE; i++) {
        Student* current = school->hash_table.buckets[i];
        while (current != NULL) {
            printf("%-20s %-20s %-15s %-6d %-6d ",
                   current->first_name, current->last_name, current->phone,
                   current->grade, current->class);

            // Print grades
            for (int j = 0; j < 10; j++) {
                printf("%d ", current->grades[j]);
            }

            printf("%-15.2f\n", current->average_grade);

            current = current->next;
        }
    }

    printf("\nTotal number of students: %d\n", school->total_students);
}

void printTopNStudentsPerCourse(School* school) {
    printf("Print top N students per course function not implemented yet.\n");
    printHeap(heapMatrix[7][8]);
}

void printUnderperformedStudents(School* school, int threshold) {
    printf("Print underperformed students function not implemented yet.\n");
}

void printAverage(School* school) {
    printf("Print average function not implemented yet.\n");
}

void exportDatabase(School* school, const char* file_name) {
    printf("Export database function not implemented yet.\n");
}

void destroySchool(School* school) {
    if (!school) return;

    // Free students in hash table
    for (int i = 0; i < HASH_SIZE; i++) {
        Student* current = school->hash_table.buckets[i];
        while (current) {
            Student* temp = current;
            current = current->next;
            free(temp);
        }
    }

    // Free students in classes
    for (int i = 0; i < MAX_GRADES; i++) {
        for (int j = 0; j < MAX_CLASSES; j++) {
            free(school->grades[i].classes[j].students);
        }
    }

    free(school);
    printf("School destroyed.\n");
}




void menu() {
    char file_name[] = "C:\\Users\\Saleh\\CLionProjects\\CheckPoint\\students_with_class.txt";
    School* school = read_data_from_file(file_name);
    if (school == NULL) {
        printf("Error\n");
        return;
    }

    int input;
    do {
        printf("\n|School Manager<::>Home|\n");
        printf("--------------------------------------------------------------------------------\n");
        printf("\t[0] |--> Insert\n");
        printf("\t[1] |--> Delete\n");
        printf("\t[2] |--> Edit\n");
        printf("\t[3] |--> Search\n");
        printf("\t[4] |--> Show All\n");
        printf("\t[5] |--> Top 10 students per course\n");
        printf("\t[6] |--> Underperformed students\n");
        printf("\t[7] |--> Average per course\n");
        printf("\t[8] |--> Export\n");
        printf("\t[9] |--> Exit\n");
        printf("\n\tPlease Enter Your Choice (0-9): ");

        if (scanf("%d", &input) != 1) {
            // Invalid input, clear the input buffer
            while (getchar() != '\n');
            printf("\nInvalid input. Please enter a number between 0 and 9.\n");
            continue;
        }

        switch (input) {
            case 0:
                insertNewStudent(school);
                break;
            case 1:
                deleteStudent(school);
                break;
            case 2:
                editStudentGrade(school);
                break;
            case 3:
                searchStudent(school);
                break;
            case 4:
                printAllStudents(school);
                break;
            case 5:
                printTopNStudentsPerCourse(school);
                break;
            case 6:
                printUnderperformedStudents(school, 15);
                break;
            case 7:
                printAverage(school);
                break;
            case 8:
                exportDatabase(school, "dataExport.txt");
                break;
            case 9:
                destroySchool(school);
                break;
            default:
                printf("\nThere is no item with symbol \"%d\". Please enter a number between 0 and 9!\n", input);
                break;
        }

        // Clear the input buffer
        while (getchar() != '\n');
    } while (input != 9);
}

int main() {
    menu();
    return 0;
}