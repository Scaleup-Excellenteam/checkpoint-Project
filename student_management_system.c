#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define HASH_SIZE 10007
#define MAX_NAME 20
#define MAX_PHONE 15
#define MAX_GRADES 12
#define MAX_CLASSES 10
#define SUBJECTS 10
#define TOP_STUDENTS 10
#define THRESHOLD 60


//structs
typedef struct Student {
    char first_name[MAX_NAME];
    char last_name[MAX_NAME];
    char phone[MAX_PHONE];
    int grade;
    int class;
    int grades[SUBJECTS];
    double average_grade;
    struct Student* next;
} Student;

typedef struct {
    Student* student;
    double average_grade;
} HeapNode;

typedef struct {
    HeapNode nodes[TOP_STUDENTS];
    int size;
} Heap;

typedef struct {
    int class_id;
    int num_students;
    Heap* top_students;
    double total_grade;
    double average_grade;
} Class;

typedef struct {
    double course_totals[SUBJECTS];
    int course_counts[SUBJECTS];
    double course_averages[SUBJECTS];
    Class classes[MAX_CLASSES];
    int grade_id;
    int num_classes;
} Grade;

typedef struct {
    Student* buckets[HASH_SIZE];
    int size;
} HashTable;

typedef struct UnderperformingStudent {
    Student* student;
    struct UnderperformingStudent* next;
} UnderperformingStudent;

typedef struct {
    UnderperformingStudent* head;
} UnderperformingList;

typedef struct {
    HashTable hash_table;
    Grade grades[MAX_GRADES];
    int num_of_grades;
    int total_students;
    UnderperformingList underperforming_students;
} School;

//functions
unsigned long hash(const char* first_name, const char* last_name);
HashTable* create_hash_table();
School* create_school();
School* read_data_from_file(const char* file_name);
void insert_student(School* school, Student* student);
void insertNewStudent(School* school);
void deleteStudent(School* school);
void editStudentGrade(School* school);
void searchStudent(School* school);
void printAllStudents(School* school);
void printTopStudentsPerCourse(School* school);
void printUnderperformedStudents(School* school, int threshold);
void printAverage(School* school);
void exportDatabase(School* school, const char* file_name);
void destroySchool(School* school);
Student* find(School* school, const char* first_name, const char* last_name);
Heap* create_heap();
void insert_or_update_heap(Heap* heap, Student* student, double average_grade);
void heapify_up(Heap* heap, int index);
void heapify_down(Heap* heap, int index);
void swap_nodes(HeapNode* a, HeapNode* b);



Heap* create_heap() {
    Heap* heap = (Heap*)malloc(sizeof(Heap));
    heap->size = 0;
    return heap;
}

void swap_nodes(HeapNode* a, HeapNode* b) {
    HeapNode temp = *a;
    *a = *b;
    *b = temp;
}

void insert_or_update_heap(Heap* heap, Student* student, double average_grade) {
    if (heap->size < TOP_STUDENTS) {
        // If the heap is not full, add the new student
        heap->nodes[heap->size].student = student;
        heap->nodes[heap->size].average_grade = average_grade;
        heap->size++;
        heapify_up(heap, heap->size - 1);
    } else if (average_grade > heap->nodes[0].average_grade) {
        // If the heap is full and the new grade is higher than the minimum in the heap,
        // replace the root (minimum) and heapify down
        heap->nodes[0].student = student;
        heap->nodes[0].average_grade = average_grade;
        heapify_down(heap, 0);
    }
}

void heapify_up(Heap* heap, int index) {
    while (index > 0) {
        int parent = (index - 1) / 2;
        if (heap->nodes[parent].average_grade <= heap->nodes[index].average_grade) {
            break;
        }
        swap_nodes(&heap->nodes[parent], &heap->nodes[index]);
        index = parent;
    }
}

void heapify_down(Heap* heap, int index) {
    int smallest = index;
    int left = 2 * index + 1;
    int right = 2 * index + 2;

    if (left < heap->size && heap->nodes[left].average_grade < heap->nodes[smallest].average_grade) {
        smallest = left;
    }

    if (right < heap->size && heap->nodes[right].average_grade < heap->nodes[smallest].average_grade) {
        smallest = right;
    }

    if (smallest != index) {
        swap_nodes(&heap->nodes[index], &heap->nodes[smallest]);
        heapify_down(heap, smallest);
    }
}

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

    Grade* grade = &school->grades[grade_index];
    Class* class = &grade->classes[class_index];

    // Calculate student's average grade
    int sum = 0;
    for (int i = 0; i < SUBJECTS; i++) {
        sum += student->grades[i];
    }
    student->average_grade = (double)sum / SUBJECTS;

    // Update grade course totals, counts, and averages
    for (int i = 0; i < SUBJECTS; i++) {
        grade->course_totals[i] += student->grades[i];
        grade->course_counts[i]++;
        grade->course_averages[i] = grade->course_totals[i] / grade->course_counts[i];
    }

    // Update class average
    class->num_students++;
    class->total_grade += student->average_grade;
    class->average_grade = class->total_grade / class->num_students;

    // Insert into top students heap
    insert_or_update_heap(class->top_students, student, student->average_grade);

    // Insert into hash table
    unsigned long index = hash(student->first_name, student->last_name);
    student->next = school->hash_table.buckets[index];
    school->hash_table.buckets[index] = student;
    school->hash_table.size++;

    school->total_students++;
}

School* create_school() {
    School* school = malloc(sizeof(School));
    if (!school) return NULL;

    memset(school, 0, sizeof(School));

    for (int i = 0; i < MAX_GRADES; i++) {
        school->grades[i].grade_id = i + 1;
        for (int j = 0; j < MAX_CLASSES; j++) {
            school->grades[i].classes[j].class_id = j + 1;
            school->grades[i].classes[j].top_students = create_heap();
            if (!school->grades[i].classes[j].top_students) {
                // Handle allocation failure
                return NULL;
            }
        }
        school->grades[i].num_classes = MAX_CLASSES;
        
        // Initialize course totals, counts, and averages
        for (int k = 0; k < SUBJECTS; k++) {
            school->grades[i].course_totals[k] = 0;
            school->grades[i].course_counts[k] = 0;
            school->grades[i].course_averages[k] = 0;
        }
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

            if(student->average_grade < THRESHOLD){
                UnderperformingStudent* new_entry = malloc(sizeof(UnderperformingStudent));
                new_entry->student = student;
                new_entry->next = school->underperforming_students.head;
                school->underperforming_students.head = new_entry;
            }
            insert_student(school, student);
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
    Student* new_student = malloc(sizeof(Student));
    if (!new_student) {
        printf("Failed to allocate memory for new student.\n");
        return;
    }

    printf("Enter student's first name: ");
    scanf("%s", new_student->first_name);

    printf("Enter student's last name: ");
    scanf("%s", new_student->last_name);

    printf("Enter student's phone number: ");
    scanf("%s", new_student->phone);

    printf("Enter student's grade (1-%d): ", MAX_GRADES);
    scanf("%d", &new_student->grade);

    printf("Enter student's class (1-%d): ", MAX_CLASSES);
    scanf("%d", &new_student->class);

    printf("Enter student's grades for %d subjects:\n", SUBJECTS);
    for (int i = 0; i < SUBJECTS; i++) {
        printf("Subject %d: ", i + 1);
        scanf("%d", &new_student->grades[i]);
    }

    // Validate input
    if (new_student->grade < 1 || new_student->grade > MAX_GRADES ||
        new_student->class < 1 || new_student->class > MAX_CLASSES) {
        printf("Invalid grade or class. Student not added.\n");
        free(new_student);
        return;
    }

    // Calculate average grade
    int sum = 0;
    for (int i = 0; i < SUBJECTS; i++) {
        sum += new_student->grades[i];
    }
    new_student->average_grade = (double)sum / SUBJECTS;
    if (new_student->average_grade < THRESHOLD) {
        UnderperformingStudent* new_entry = malloc(sizeof(UnderperformingStudent));
        new_entry->student = new_student;
        new_entry->next = school->underperforming_students.head;
        school->underperforming_students.head = new_entry;
    } 

    // Insert the new student
    insert_student(school, new_student);

    printf("Student %s %s added successfully.\n", new_student->first_name, new_student->last_name);
}

void deleteStudent(School* school) {
    char first_name[MAX_NAME];
    char last_name[MAX_NAME];
    
    printf("Enter the first name of the student to delete: ");
    scanf("%s", first_name);
    printf("Enter the last name of the student to delete: ");
    scanf("%s", last_name);

    unsigned long index = hash(first_name, last_name);
    Student* current = school->hash_table.buckets[index];
    Student* prev = NULL;
    
    while (current != NULL) {
        if (strcasecmp(current->first_name, first_name) == 0 && 
            strcasecmp(current->last_name, last_name) == 0) {
            
            // Remove from underperforming list if present
            if (current->average_grade < THRESHOLD) {
                UnderperformingStudent** current_underperf = &school->underperforming_students.head;
                while (*current_underperf != NULL) {
                    if ((*current_underperf)->student == current) {
                        UnderperformingStudent* to_remove = *current_underperf;
                        *current_underperf = (*current_underperf)->next;
                        free(to_remove);
                        break;
                    }
                    current_underperf = &(*current_underperf)->next;
                }
            }
            
            // Remove from hash table
            if (prev == NULL) {
                school->hash_table.buckets[index] = current->next;
            } else {
                prev->next = current->next;
            }

            // Update grade statistics
            Grade* grade = &school->grades[current->grade - 1];
            for (int i = 0; i < SUBJECTS; i++) {
                grade->course_totals[i] -= current->grades[i];
                grade->course_counts[i]--;
                if (grade->course_counts[i] > 0) {
                    grade->course_averages[i] = grade->course_totals[i] / grade->course_counts[i];
                } else {
                    grade->course_averages[i] = 0;
                }
            }

            // Update class statistics
            Class* class = &grade->classes[current->class - 1];
            class->num_students--;
            class->total_grade -= current->average_grade;
            if (class->num_students > 0) {
                class->average_grade = class->total_grade / class->num_students;
            } else {
                class->average_grade = 0;
            }

            // Update top students heap (remove the student if present)
            Heap* heap = class->top_students;
            for (int i = 0; i < heap->size; i++) {
                if (heap->nodes[i].student == current) {
                    heap->nodes[i] = heap->nodes[heap->size - 1];
                    heap->size--;
                    heapify_down(heap, i);
                    break;
                }
            }

            school->total_students--;

            printf("Student %s %s has been deleted.\n", first_name, last_name);
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }

    printf("Student %s %s not found.\n", first_name, last_name);
}

void editStudentGrade(School* school) {
    char first_name[MAX_NAME];
    char last_name[MAX_NAME];
    int subject, new_grade;
    
    printf("Enter the first name of the student: ");
    scanf("%s", first_name);
    printf("Enter the last name of the student: ");
    scanf("%s", last_name);

    Student* student = find(school, first_name, last_name);
    if (student == NULL) {
        printf("Student not found.\n");
        return;
    }

    printf("Current grades:\n");
    for (int i = 0; i < SUBJECTS; i++) {
        printf("Subject %d: %d\n", i + 1, student->grades[i]);
    }

    printf("Enter the subject number to edit (1-%d): ", SUBJECTS);
    scanf("%d", &subject);
    if (subject < 1 || subject > SUBJECTS) {
        printf("Invalid subject number.\n");
        return;
    }
    subject--; // Adjust for 0-based indexing

    printf("Enter the new grade: ");
    scanf("%d", &new_grade);
    if (new_grade < 0 || new_grade > 100) {
        printf("Invalid grade. Please enter a grade between 0 and 100.\n");
        return;
    }

    int old_grade = student->grades[subject];
    student->grades[subject] = new_grade;

    // Recalculate student's average
    int sum = 0;
    for (int i = 0; i < SUBJECTS; i++) {
        sum += student->grades[i];
    }
    double old_average = student->average_grade;
    student->average_grade = (double)sum / SUBJECTS;

    if (student->average_grade < THRESHOLD && old_average >= THRESHOLD){
        UnderperformingStudent* new_entry = malloc(sizeof(UnderperformingStudent));
        new_entry->student = student;
        new_entry->next = school->underperforming_students.head;
        school->underperforming_students.head = new_entry;
    } else if(student->average_grade >= THRESHOLD && old_average < THRESHOLD){    
        UnderperformingStudent** current_underperf = &school->underperforming_students.head;
        while (*current_underperf != NULL) {
            if ((*current_underperf)->student == student) {
                UnderperformingStudent* to_remove = *current_underperf;
                *current_underperf = (*current_underperf)->next;
                free(to_remove);
                break;
            }
            current_underperf = &(*current_underperf)->next;
        }     
    }

    // Update grade statistics
    Grade* grade = &school->grades[student->grade - 1];
    grade->course_totals[subject] += (new_grade - old_grade);
    grade->course_averages[subject] = grade->course_totals[subject] / grade->course_counts[subject];

    // Update class statistics
    Class* class = &grade->classes[student->class - 1];
    class->total_grade += (student->average_grade - old_average);
    class->average_grade = class->total_grade / class->num_students;

    // Update top students heap
    Heap* heap = class->top_students;
    for (int i = 0; i < heap->size; i++) {
        if (heap->nodes[i].student == student) {
            heap->nodes[i].average_grade = student->average_grade;
            heapify_down(heap, i);
            heapify_up(heap, i);
            break;
        }
    }

    printf("Grade updated successfully.\n");
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

void printUnderperformedStudents(School* school, int threshold) {
    if (!school || !school->underperforming_students.head) {
        printf("No underperforming students.\n");
        return;
    }

    printf("\n%-20s %-20s %-15s %-6s %-6s %-15s\n", 
           "First Name", "Last Name", "Phone", "Grade", "Class", "Average Grade");
    printf("--------------------------------------------------------------------------------\n");

    UnderperformingStudent* current = school->underperforming_students.head;
    while (current != NULL) {
        Student* student = current->student;
        printf("%-20s %-20s %-15s %-6d %-6d %-15.2f\n", 
               student->first_name, student->last_name, student->phone, 
               student->grade, student->class, student->average_grade);
        current = current->next;
    }
}

void printAverage(School* school) {
    if (!school || school->total_students == 0) {
        printf("No students in the school.\n");
        return;
    }

    for (int grade = 0; grade < MAX_GRADES; grade++) {
        Grade* grade_ptr = &school->grades[grade];
        
        if (grade_ptr->num_classes > 0) {
            printf("\nGrade %d Averages:\n", grade + 1);
            printf("------------------\n");
            for (int subject = 0; subject < SUBJECTS; subject++) {
                if (grade_ptr->course_counts[subject] > 0) {
                    printf("Course %d: %.2f\n", subject + 1, grade_ptr->course_averages[subject]);
                } else {
                    printf("Course %d: No data\n", subject + 1);
                }
            }
            printf("\n");
        }
    }
}

void exportDatabase(School* school, const char* file_name) {
    FILE* file = fopen(file_name, "w");
    if (!file) {
        printf("Error opening file for writing.\n");
        return;
    }

    fprintf(file, "Hashtable Data:\n");
    for (int i = 0; i < HASH_SIZE; i++) {
        Student* current = school->hash_table.buckets[i];
        while (current) {
            fprintf(file, "Name: %s %s, Phone: %s, Grade: %d, Class: %d, Average Grade: %.2f, Grades: ",
                    current->first_name, current->last_name, current->phone,
                    current->grade, current->class, current->average_grade);
            for (int j = 0; j < SUBJECTS; j++) {
                fprintf(file, " %d", current->grades[j]);
            }
            fprintf(file, "\n");
            current = current->next;
        }
    }

    fprintf(file, "\nHeap Data:\n");
    for (int grade = 0; grade < MAX_GRADES; grade++) {
        for (int class = 0; class < MAX_CLASSES; class++) {
            Heap* heap = school->grades[grade].classes[class].top_students;
            fprintf(file, "Grade %d, Class %d:\n", grade + 1, class + 1);
            for (int i = 0; i < heap->size; i++) {
                Student* student = heap->nodes[i].student;
                fprintf(file, "Name: %s %s, Phone: %s, Grade: %d, Class: %d, Average Grade: %.2f\n",
                        student->first_name, student->last_name, student->phone,
                        student->grade, student->class, student->average_grade);
            }
        }
    }

    fclose(file);
    printf("Database exported successfully to %s.\n", file_name);
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

    // Free the underperforming students list
    UnderperformingStudent* current_underperf = school->underperforming_students.head;
    while (current_underperf != NULL) {
        UnderperformingStudent* to_free = current_underperf;
        current_underperf = current_underperf->next;
        free(to_free);
    }

    // Free heap structures
    for (int i = 0; i < MAX_GRADES; i++) {
        for (int j = 0; j < MAX_CLASSES; j++) {
            free(school->grades[i].classes[j].top_students);
        }
    }

    free(school);
    printf("School destroyed.\n");
}



void printTopStudentsPerCourse(School* school) {
    for (int grade = 1; grade <= MAX_GRADES; grade++) {
        for (int class = 1; class <= MAX_CLASSES; class++) {
            Grade* grade_ptr = &school->grades[grade - 1];
            Class* class_ptr = &grade_ptr->classes[class - 1];
            
            // Skip empty classes
            if (class_ptr->num_students == 0) {
                continue;
            }

            printf("\nTop %d students in Grade %d, Class %d:\n", TOP_STUDENTS, grade, class);
            printf("----------------------------------------------------\n");

            // Create a copy of the heap to sort
            Heap temp_heap = *class_ptr->top_students;

            // Sort the heap (extract min repeatedly)
            HeapNode sorted[TOP_STUDENTS];
            int sort_size = temp_heap.size;
            for (int i = sort_size - 1; i >= 0; i--) {
                sorted[i] = temp_heap.nodes[0];
                temp_heap.nodes[0] = temp_heap.nodes[temp_heap.size - 1];
                temp_heap.size--;
                heapify_down(&temp_heap, 0);
            }

            // Print sorted students (in descending order of grades)
            for (int i = 0; i < sort_size; i++) {
                printf("%d. %s %s - Average Grade: %.2f\n", i + 1, 
                       sorted[i].student->first_name, 
                       sorted[i].student->last_name, 
                       sorted[i].average_grade);
            }

            printf("\nClass Average: %.2f\n", class_ptr->average_grade);
            printf("----------------------------------------------------\n");
        }
    }
}

void menu() {
    char file_name[] = "students_with_class.txt";
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
                printTopStudentsPerCourse(school);
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