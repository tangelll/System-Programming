#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
void createFile(char *filename);
void addStudentGrade(char *name ,char*surname, char *grade,char *filename);
void searchStudent(char *name ,char*surname,char*filename);
void sortAll(char *filename);
void showAll(char *filename);
void listGrades(char *filename);
void listSome(int numEntries, int pageNumber, char *filename);
int countEntries(char *filename);
char filename[50] = "grades.txt";
void logMessage(const char *message, FILE *log_fp);

FILE *log_fp;
#define LOG_FILE "log.txt"

int main() {

  log_fp = fopen(LOG_FILE, "a"); // Open log file in append mode

  if (log_fp == NULL) {
    perror("Error opening log file");
    exit(1);
  }

  char command[100];
  printf("Oncelikle gtuStudentGrades komutunu kullanarak bir dosya olsuturun dosya olusturmadan diger komutlari deneyemezsiniz yada disaridan dosya atmayin\n");
  while (1) {
    printf("> ");
    scanf("%s", command);
    
    



    // Komut karşılaştırması
    if (strcmp(command, "gtuStudentGrades") == 0) {
      scanf("%s",filename);
      createFile(filename);
    } else if (strcmp(command, "addStudentGrade") == 0) {
      char name[50], grade[10],  surname [50];
      scanf("%s %s %s", name ,surname, grade);
      addStudentGrade(name,surname, grade,filename);
    } else if (strcmp(command, "searchStudent") == 0) {
      char name[50],surname [50];
      scanf("%s %s", name,surname);
      searchStudent(name ,surname,filename);
    } else if (strcmp(command, "sortAll") == 0) {
      sortAll(filename);
    } else if (strcmp(command, "showAll") == 0) {
      showAll(filename);
    } else if (strcmp(command, "listGrades") == 0) {
      listGrades(filename);
    } else if (strcmp(command, "listSome") == 0) {
      int numEntries, pageNumber;
      scanf("%d %d %s", &numEntries, &pageNumber, filename);
      listSome(numEntries, pageNumber, filename);
    } else if(strcmp(command, "gtuStudentGrades") == 0  &&  scanf("%s",filename) == 0 ){
      
        printf("\nAvailable commands:\n");
        printf("1. showAll filename: Display all student grades from the specified file\n");
        printf("2. listGrades filename [numEntries]: List the first N (or all) entries from the file\n");
        printf("3. listSome filename numEntriesPerPage pageNumber: List entries by page number and entries per page\n");
        printf("4. exit: Exit the program\n");

    } 
      else if (strcmp(command, "exit") == 0) {
      break;

    } 

  
    
  }

}

// Fonksiyonların kodları ...

void logMessage(const char *message, FILE *log_fp) {
  time_t t = time(NULL);
  struct tm *tm = localtime(&t);

  // Get current date and time
  char timestamp[50];
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm);

  // Write message to log file
  fprintf(log_fp, "[%s] %s\n", timestamp, message);

  // Flush log file buffer
  fflush(log_fp);
}


void createFile(char *filename){
  // Create a child process using fork
  pid_t pid = fork();

  if (pid < 0) {
    perror("fork failed");
    exit(1);
  }

  // Child process - Create the file
  if (pid == 0) {
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
      perror("Error creating file");
      exit(1);
    }
    printf("File '%s' created successfully.\n", filename);
    fclose(fp);
  }

  // Parent process - Wait for child and continue
  wait(NULL);
}
void addStudentGrade(char *name, char*surname,char *grade,char *filename){

  // Çocuk işlemi oluşturmak için fork kullanın
  pid_t pid = fork();

  if (pid < 0) {
    perror("fork başarısız oldu");
    exit(1);
  }

  // Çocuk işlemi - Dosyaya not ekleyin
  if (pid == 0) {
    FILE *fp = fopen(filename, "a");
    if (fp == NULL) {
      perror("Dosya açma hatası");
      exit(1);
    }

    // Biçimlendirilmiş dizeyi dosyaya yazın
    fprintf(fp, "%s %s %s\n", name, surname,grade);
    fclose(fp);

    printf("'%s %s' isimli öğrenciye '%s' notu eklendi.\n", name, surname,grade);
    exit(0);
  }

  // Ana işlem - Çocuğu bekleyin ve devam edin
  wait(NULL);
}

void searchStudent(char *name,char *surname,char * filename){
  // Dosya açma

  // Çocuk işlemi oluşturmak için fork kullanın
  pid_t pid = fork();

  if (pid < 0) {
    perror("fork başarısız oldu");
    exit(1);
  }

  printf("%s",filename);
  FILE *fp = fopen(filename, "r");
  if (fp == NULL) {
    printf("Dosya açılamadı!\n");
    return;
  }

  // Arama yapılacak satır
  char line[100];

  // Dosya sonuna kadar okuma
  while (fgets(line, sizeof(line), fp)) {
    // İsim ve soyisimi ayırma
    char *name_in_file = strtok(line, " ");
    char *surname_in_file = strtok(NULL, " ");

    // Eşleşme kontrolü
    if (strcmp(name, name_in_file) == 0 && strcmp(surname, surname_in_file) == 0) {
      // Notu alma
      char *grade = strtok(NULL, " ");

      // Sonucu yazdırma
      printf("%s %s'nin notu: %s\n", name, surname, grade);

      // Dosyayı kapatma
      fclose(fp);

      // Fonksiyondan çıkma
      return;
    }

    
  }

  // Eşleşme bulunamadı mesajı
  printf("%s %s isimli öğrenci bulunamadı!\n", name, surname);

  // Dosyayı kapatma
  fclose(fp);
   // Ana işlem - Çocuğu bekleyin ve devam edin
  wait(NULL);

}

// Öğrenci bilgisi için struct
typedef struct student {
  char name[50];
  float grade;
} Student;

// Karşılaştırma fonksiyonu (grade'e göre sıralama)
int compareGrade(const void *a, const void *b) {
  Student *studentA = (Student *)a;
  Student *studentB = (Student *)b;
  return (studentA->grade > studentB->grade) - (studentA->grade < studentB->grade);
}

// Dosyayı okur ve öğrenci bilgilerini içeren bir dizi oluşturur
Student *readStudents(char *filename, int *n) {
  FILE *file = fopen(filename, "r");
  if (file == NULL) {
    printf("Hata: Dosya '%s' bulunamadı.\n", filename);
    return NULL;
  }

  // Dosyadaki satır sayısını bul
  int lines = 0;
  char line[100];
  while (fgets(line, sizeof(line), file)) {
    lines++;
  }
  fclose(file);

  // Öğrenci dizisini ayarla
  *n = lines;
  Student *students = malloc(sizeof(Student) * lines);

  // Dosyayı tekrar aç ve öğrenci bilgilerini oku
  file = fopen(filename, "r");
  for (int i = 0; i < lines; i++) {
    fscanf(file, "%[^,],%f", students[i].name, &students[i].grade);
  }
  fclose(file);

  return students;
}

// Öğrenci bilgilerini sıralar ve yazdırır
void sortAndPrint(Student *students, int n) {
  // Öğrenci listesini grade'e göre sırala
  qsort(students, n, sizeof(Student), compareGrade);

  // Sıralama yapılmış öğrenci bilgilerini yazdır
  for (int i = 0; i < n; i++) {
    printf("%s: %.2f\n", students[i].name, students[i].grade);
  }
}

void sortAll(char *filename) {

   // Çocuk işlemi oluşturmak için fork kullanın
  pid_t pid = fork();

  if (pid < 0) {
    perror("fork başarısız oldu");
    exit(1);
  }

  int n;
  Student *students = readStudents(filename, &n);
  if (students == NULL) {
    return;
  }

  sortAndPrint(students, n);
  logMessage("SortAll did", log_fp);


  free(students);
  // Ana işlem - Çocuğu bekleyin ve devam edin
  wait(NULL);
}



int countEntries(char *filename){
  FILE *fp = fopen(filename, "r");
  if (fp == NULL) {
    perror("Dosya açma hatası");
    exit(1);
  }

  int numEntries = 0;
  char line[100];  // Maksimum satır uzunluğu için uygun bir değer seçin

  // Dosyayı satır satır okuma
  while (fgets(line, sizeof(line), fp) != NULL) {
    numEntries++;
  }

  fclose(fp);
  return numEntries;
}

void showAll(char *filename){


   // Çocuk işlemi oluşturmak için fork kullanın
  pid_t pid = fork();

  if (pid < 0) {
    perror("fork başarısız oldu");
    exit(1);
  }

  FILE *fp = fopen(filename, "r");
  if (fp == NULL) {
    perror("Dosya açma hatası");
    exit(1);
  }

  char line[100];
  while (fgets(line, sizeof(line), fp) != NULL) {
    printf("%s", line);
  }

  logMessage("All Student Grades Displayed", log_fp);

  fclose(fp);

  // Ana işlem - Çocuğu bekleyin ve devam edin
  wait(NULL);
}


void listGrades(char *filename){

   // Çocuk işlemi oluşturmak için fork kullanın
  pid_t pid = fork();

  if (pid < 0) {
    perror("fork başarısız oldu");
    exit(1);
  }


  FILE *fp = fopen(filename, "r");
  if (fp == NULL) {
    perror("Dosya açma hatası");
    exit(1);
  }

  char line[100];
  int count = 0;

  // İlk 5 satırı okuma ve yazdırma
  while (fgets(line, sizeof(line), fp) != NULL && count < 5) {
    printf("%s", line);
    count++;
  }

  fclose(fp);
  // Ana işlem - Çocuğu bekleyin ve devam edin
  wait(NULL);
}


void listSome(int numEntriesPerEntry, int pageNumber, char *filename){

     // Çocuk işlemi oluşturmak için fork kullanın
  pid_t pid = fork();

  if (pid < 0) {
    perror("fork başarısız oldu");
    exit(1);
  }

  FILE *fp = fopen(filename, "r");
  if (fp == NULL) {
    perror("Dosya açma hatası");
    exit(1);
  }

  char line[100];

  struct student student;
  int startEntry = (pageNumber - 1) * numEntriesPerEntry;
  int count = 0;
  int skippedLines = 0;

  // Belirtilen sayıda satırı okuma ve yazdırma
  while (fgets(line, sizeof(line), fp) != NULL) {
    if (count >= startEntry && count < startEntry + numEntriesPerEntry) {
      printf("%s", line);
    } else if (count < startEntry) {
      skippedLines++;
    }
    count++;
  }

  if (count == 0) {
    printf("Dosyada herhangi bir kayıt bulunamadı.\n");
  } else if (skippedLines > 0) {
    printf("%d satır atlanarak %d satır listelendi.\n", skippedLines, numEntriesPerEntry);
  }

  fclose(fp);

  // Ana işlem - Çocuğu bekleyin ve devam edin
  wait(NULL);
}
