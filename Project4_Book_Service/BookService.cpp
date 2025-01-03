#include <iostream>
#include <iterator>
#include <map>
#include <unordered_map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

class RentalInfo;
class Book;

struct DateStruct {
    int year, month, day;

    DateStruct(int year, int month, int day)
        : year{ year }, month{ month }, day{ day } {
    }

    string getDateString() const {
        return to_string(year) + "-" +
            (month < 10 ? "0" + to_string(month) : to_string(month)) + "-" +
            (day < 10 ? "0" + to_string(day) : to_string(day));
    }

    bool operator<(const DateStruct& date) const {
        if (year != date.year) {
            return year < date.year;
        }
        else if (month < date.month) {
            return month < date.month;
        }
        else {
            return day < date.day;
        }
    }

    bool operator==(const DateStruct& date) const {
        return year == date.year && month == date.month && day == date.day;
    }
};

struct RentalDTO {
    string borrower;
    string phone;
    DateStruct date;

    RentalDTO(string borrower, string phone, DateStruct date)
        : borrower{ borrower }, phone{ phone }, date(date) {
    };
};

class Idisplayable {
public:
    virtual ~Idisplayable() = default;
    virtual void displaySelf() const = 0;
};

class Book : public Idisplayable {
private:
    int id;
    string title;
    string author;

public:
    shared_ptr<RentalInfo> rentalInfo;

    Book(int id, string title, string author)
        : id{ id }, title{ title }, author{ author }, rentalInfo{ nullptr } {
        cout << "생성됨. 책번호: " << id << endl;
    }

    ~Book() {
        cout << "소멸함, 책번호:" << id << endl;
    }
    int getId() const {
        return id;
    }
    string getTitle() const {
        return title;
    }
    string getAuthor() const {
        return author;
    }
    void displaySelf() const override;
};

class RentalInfo : public Idisplayable {
private:
    string borrower;
    string phone;
    DateStruct returnDate;

public:
    weak_ptr<Book> book;
    RentalInfo(shared_ptr<Book> book, RentalDTO rentalDTO)
        : borrower{ rentalDTO.borrower }, phone{ rentalDTO.phone },
        returnDate(rentalDTO.date), book(book) {
    }

    string getBorrower() const {
        return borrower;
    }

    DateStruct getReturnDate() const {
        return returnDate;
    }

    void displaySelf() const override;
};

void Book::displaySelf() const {
    string dateString =
        rentalInfo
        ? "대여중\n반납일: " + rentalInfo->getReturnDate().getDateString()
        : "대여가능";

    cout << "-----책정보-----" << endl;
    cout << "번호: " << id << endl;
    cout << "제목: " << title << endl;
    cout << "작가: " << author << endl;
    cout << "상태: " << dateString << endl;
    cout << endl;
}

void RentalInfo::displaySelf() const {
    auto lockedBook = book.lock();

    cout << "-----대여정보-----" << endl;
    cout << "책 제목: " << lockedBook->getTitle() << endl;
    cout << "빌린사람: " << borrower << endl;
    cout << "전화번호: " << phone << endl;
    cout << "반납일: " << returnDate.getDateString() << endl;
    cout << endl;
}

class BookManager {
private:
    unique_ptr<vector<shared_ptr<Book>>> books;
    unique_ptr<unordered_map<int, shared_ptr<Book>>> idIndex;
    unique_ptr<unordered_map<string, vector<shared_ptr<Book>>>> titleIndex;
    unique_ptr<unordered_map<string, vector<shared_ptr<Book>>>> authorIndex;
    void sortBooksById();

public:
    BookManager() {
        books = make_unique<vector<shared_ptr<Book>>>();
        idIndex = make_unique<unordered_map<int, shared_ptr<Book>>>();
        titleIndex =
            make_unique<unordered_map<string, vector<shared_ptr<Book>>>>();
        authorIndex =
            make_unique<unordered_map<string, vector<shared_ptr<Book>>>>();
    }

    void addBook(string title, string author);
    vector<shared_ptr<Book>> getAllBooks();
    vector<shared_ptr<Book>> getBooksByTitle(string title);
    vector<shared_ptr<Book>> getBooksByAuthor(string author);
    shared_ptr<Book> getBookById(int id);
};

class RentalManager {
private:
    unique_ptr<vector<shared_ptr<RentalInfo>>> rentals;
    unique_ptr<unordered_map<string, vector<shared_ptr<RentalInfo>>>>
        borrowerIndex;
    unique_ptr<multimap<DateStruct, shared_ptr<RentalInfo>>> returnDateIndex;
    void rentalBook(shared_ptr<Book> book, RentalDTO rentalDTO);
    void returnBook(shared_ptr<Book> book);

public:
    RentalManager() {
        rentals = make_unique<vector<shared_ptr<RentalInfo>>>();

        borrowerIndex = make_unique<
            unordered_map<string, vector<shared_ptr<RentalInfo>>>>();

        returnDateIndex =
            make_unique<multimap<DateStruct, shared_ptr<RentalInfo>>>();
    }

    void returnBookById(int bookId, BookManager& bookManager);
    void rentalBookById(int bookId, RentalDTO rentalDTO,
        BookManager& bookManager);
    void rentalBookByTitle(string title, RentalDTO rentalDTO,
        BookManager& bookManager);
    vector<shared_ptr<RentalInfo>> getAllRentals();
    vector<shared_ptr<RentalInfo>> getRentalsByBorrower(string borrower);
    vector<shared_ptr<RentalInfo>>
        getDelayedRentalsByReturnDate(DateStruct returnDate);
};

void BookManager::sortBooksById() {
    sort(books->begin(), books->end(), [](auto b1, auto b2) -> bool {
        int b1Id = b1->getId();
        int b2Id = b2->getId();
        return b1Id < b2Id;
        });
}

void BookManager::addBook(string title, string author) {
    // 안해도 유지될거 같긴 한데..
    // sortBooksById();
    int newId = 1;
    if (books->size() > 0) {
        newId = books->back()->getId() + 1;
    }

    // Book 생성
    shared_ptr<Book> newBook = make_shared<Book>(newId, title, author);

    // idIndex에 추가
    auto idIndexIt = idIndex->find(newId);

    if (idIndexIt != idIndex->end()) {
        cout << "이미 등록된 책입니다." << endl;
        return;
    }
    else {

        idIndex->insert({ newId, newBook });
    }

    // books에 추가
    books->push_back(newBook);

    // titleIndex에 추가
    auto titleIndexIt = titleIndex->find(title);
    if (titleIndexIt != titleIndex->end()) {
        titleIndexIt->second.push_back(newBook);
    }
    else {
        titleIndex->insert({ title, {newBook} });
    }

    // authorIndex에 추가
    auto authorIndexIt = authorIndex->find(author);
    if (authorIndexIt != authorIndex->end()) {
        authorIndexIt->second.push_back(newBook);
    }
    else {
        authorIndex->insert({ author, {newBook} });
    }

    cout << "----책 추가 완료, 아래는 추가된 책----" << endl;
    newBook->displaySelf();
}

vector<shared_ptr<Book>> BookManager::getAllBooks() {
    vector<shared_ptr<Book>> result(*books);
    return result;
}

vector<shared_ptr<Book>> BookManager::getBooksByTitle(string title) {
    auto result = vector<shared_ptr<Book>>();

    auto targetIt = titleIndex->find(title);
    if (targetIt != titleIndex->end()) {
        for (auto book : targetIt->second) {
            result.push_back(book);
        }
    }

    return result;
}

vector<shared_ptr<Book>> BookManager::getBooksByAuthor(string author) {
    vector<shared_ptr<Book>> result;

    auto targetIt = authorIndex->find(author);
    if (targetIt != titleIndex->end()) {
        for (auto book : targetIt->second) {
            result.push_back(book);
        }
    }

    return result;
}

shared_ptr<Book> BookManager::getBookById(int id) {
    auto targetIt = idIndex->find(id);

    if (targetIt != idIndex->end()) {
        return targetIt->second;
    }
    return nullptr;
}

void RentalManager::rentalBook(shared_ptr<Book> book, RentalDTO rentalDTO) {
    auto [borrower, phone, returnDate] = rentalDTO;
    auto newRentalInfo = make_shared<RentalInfo>(book, rentalDTO);
    book->rentalInfo = newRentalInfo;

    // rentals에 추가
    rentals->push_back(newRentalInfo);

    // borrowerIndex에 추가
    auto borrowerIndexIt = borrowerIndex->find(borrower);
    if (borrowerIndexIt != borrowerIndex->end()) {
        borrowerIndexIt->second.push_back(newRentalInfo);
    }
    else {
        borrowerIndex->insert({ borrower, {newRentalInfo} });
    }

    // returnDateIndex에 추가
    returnDateIndex->insert({ returnDate, newRentalInfo });

    cout << "----대여완료, 대여정보 출력----" << endl;
    newRentalInfo->displaySelf();
}

void RentalManager::returnBook(shared_ptr<Book> book) {
    auto targetRental = book->rentalInfo;
    string targetBorrower = targetRental->getBorrower();
    DateStruct targetReturnDate = targetRental->getReturnDate();

    // rentals 에서 제거
    rentals->erase(remove(rentals->begin(), rentals->end(), targetRental),
        rentals->end());

    // borrowerIndex에서 제거
    auto& targetVector = borrowerIndex->at(targetBorrower);
    targetVector.erase(
        remove(targetVector.begin(), targetVector.end(), targetRental),
        targetVector.end());

    // returnDateIndex에서 제거
    auto [targetBeginIt, targetEndIt] =
        returnDateIndex->equal_range(targetReturnDate);
    vector<shared_ptr<RentalInfo>> backupVector;
    for (auto it = targetBeginIt; it != targetEndIt; it++) {
        auto backupRentalInfo = it->second;
        if (targetRental != backupRentalInfo) {
            backupVector.push_back(it->second);
        }
    }
    returnDateIndex->erase(targetReturnDate);
    for (auto el : backupVector) {
        returnDateIndex->insert({ targetReturnDate, el });
    }

    // book -> rentalInfo 참조 해제
    book->rentalInfo = nullptr;

    cout << "반납 완료." << endl;
}

void RentalManager::returnBookById(int bookId, BookManager& bookManager) {
    auto targetBook = bookManager.getBookById(bookId);
    if (!targetBook) {
        cout << "없는 책번호" << endl;
        return;
    }

    if (targetBook->rentalInfo) {
        returnBook(targetBook);
        return;
    }

    cout << "대여되지 않은 책" << endl;
}

void RentalManager::rentalBookById(int bookId, RentalDTO rentalDTO,
    BookManager& bookManager) {
    auto targetBook = bookManager.getBookById(bookId);
    if (!targetBook) {
        cout << "없는 책번호" << endl;
        return;
    }

    if (!targetBook->rentalInfo) {
        rentalBook(targetBook, rentalDTO);
        return;
    }

    cout << "이미 대여된 책" << endl;
}

void RentalManager::rentalBookByTitle(string title, RentalDTO rentalDTO,
    BookManager& bookManager) {
    auto targetBooks = bookManager.getBooksByTitle(title);

    for (auto book : targetBooks) {
        if (!book->rentalInfo) {
            rentalBook(book, rentalDTO);
            return;
        }
    }
    cout << "모두 대여중이거나 없는 책" << endl;
}

vector<shared_ptr<RentalInfo>> RentalManager::getAllRentals() {
    vector<shared_ptr<RentalInfo>> result;
    for (int i = 0; i < rentals->size(); i++) {
        result.push_back(rentals->at(i));
    }
    return result;
}

vector<shared_ptr<RentalInfo>>
RentalManager::getRentalsByBorrower(string borrower) {
    vector<shared_ptr<RentalInfo>> result;
    auto borrowerIndexIt = borrowerIndex->find(borrower);
    if (borrowerIndexIt != borrowerIndex->end()) {
        for (auto rentalInfo : borrowerIndexIt->second) {
            result.push_back(rentalInfo);
        }
    }
    else {
        cout << "이 사람은 대여중이지 않음." << endl;
    }

    return result;
}

vector<shared_ptr<RentalInfo>>
RentalManager::getDelayedRentalsByReturnDate(DateStruct returnDate) {
    vector<shared_ptr<RentalInfo>> result;
    auto upperBoundIt = returnDateIndex->upper_bound(returnDate);

    for (auto it = returnDateIndex->begin(); it != upperBoundIt; it++) {
        result.push_back(it->second);
    }

    if (result.empty()) {
        cout << "이 날짜 이전 날짜로 반납해야 되는 책이 없음." << endl;
    }

    return result;
}

class BookService {
private:
    enum MainMode {
        BOOK_SEARCH = 1,
        RENT,
        RETURN,
        RENTAL_INFO_SEARCH,
        ADD_BOOK,
        PROGRAM_END
    };
    enum BookSearchMode { ALL_BOOKS = 1, TITLE, AUTHOR };
    enum RentalSearchMode { ALL_RENTALS = 1, BORROWER, RETURN_DATE };

    BookManager& bookManager;
    RentalManager& rentalManager;
    unique_ptr<vector<shared_ptr<Idisplayable>>> displayables;

    int getInputInteger(int min, int max);
    string getInputString();

    void initBuffer() {
        displayables->clear();
    }

    void displayMainMode();
    void displayBookSearchMode();
    void displayBookSerachAllBooks();
    void displayBookSearchTitle();
    void displayBookSearchAuthor();
    void displayRent();
    void displayReturn();
    void displayRentalSearchMode();
    void displayRentalSearchAllRentals();
    void displayRentalSearchBorrower();
    void displayRentalSearchReturnDate();
    void displayAddBook();

public:
    BookService(BookManager& bookManager, RentalManager& rentalManager)
        : bookManager{ bookManager }, rentalManager{ rentalManager },
        displayables{ make_unique<vector<shared_ptr<Idisplayable>>>() } {
    };

    void displayAllBuffer() {
        for (auto displayable : *displayables) {
            displayable->displaySelf();
        }
        initBuffer();
    }

    void setToDisplay(shared_ptr<Idisplayable> displayable) {
        displayables->push_back(displayable);
    }

    void route();
};

int BookService::getInputInteger(int min, int max) {
    int result;
    stringstream ss;

    while (true) {
        cout << "입력: ";
        int tmp;
        string input;
        cin >> input;

        ss.str(input);
        if (ss >> tmp) {
            if (ss.eof()) {
                if (tmp >= min && tmp <= max) {
                    result = tmp;
                    break;
                }
                else {
                    cout << "입력값은 " << min << "에서 " << max << "까지"
                        << endl
                        << endl;
                }
            }
        }
        else {
            cout << "잘못된 입력. 숫자를 입력해주세요." << endl << endl;
        }
        ss.clear();
        ss.str("");
    }

    return result;
}

string BookService::getInputString() {
    string result;
    cout << "입력: ";
    cin >> result;
    return result;
}

void BookService::displayMainMode() {
    cout << endl;
    cout << "----무엇을 하시겠습니까?----" << endl;
    cout << "1. 도서 검색 2. 도서 대여 3. 도서 반납 4. 대여정보 검색 5. 도서 "
        "등록 6. 나가기 "
        << endl;
}
void BookService::displayBookSearchMode() {
    cout << "----검색 방법을 선택하세요.----" << endl;
    cout << "1. 전체 출력 2. 제목 3. 작가";
}
void BookService::displayBookSerachAllBooks() {
    vector<shared_ptr<Book>> books = bookManager.getAllBooks();
    for (auto book : books) {
        setToDisplay(book);
    }
    displayAllBuffer();
}
void BookService::displayBookSearchTitle() {
    cout << "책 제목을 입력하세요." << endl;
    string title = getInputString();
    vector<shared_ptr<Book>> books = bookManager.getBooksByTitle(title);
    for (auto book : books) {
        setToDisplay(book);
    }
    displayAllBuffer();
}
void BookService::displayBookSearchAuthor() {
    cout << "작가명을 입력하세요." << endl;
    string author = getInputString();
    vector<shared_ptr<Book>> books = bookManager.getBooksByAuthor(author);
    for (auto book : books) {
        setToDisplay(book);
    }
    displayAllBuffer();
}
void BookService::displayRent() {
    cout << "책 제목, 대여자 이름, 휴대폰 번호, 날짜를 입력하세요." << endl;
    cout << "----책 제목----" << endl;
    string title = getInputString();
    cout << "----대여자 이름----" << endl;
    string borrower = getInputString();
    cout << "----휴대폰 번호----" << endl;
    string phone = getInputString();

    cout << "----년도 입력----" << endl;
    int year = getInputInteger(2025, 2100);
    cout << "----월 입력----" << endl;
    int month = getInputInteger(1, 12);
    cout << "----일 입력----" << endl;
    int day = getInputInteger(1, 31);

    rentalManager.rentalBookByTitle(
        title, RentalDTO(borrower, phone, DateStruct(year, month, day)),
        bookManager);
}
void BookService::displayReturn() {
    cout << "반납할 책 번호를 입력하세요." << endl;
    int id = getInputInteger(1, 10000);
    rentalManager.returnBookById(id, bookManager);
}
void BookService::displayRentalSearchMode() {
    cout << "----검색 방법을 선택하세요.----" << endl;
    cout << "1. 전체 출력 2. 대여자 이름 3. 반납일" << endl;
}
void BookService::displayRentalSearchAllRentals() {
    cout << "----모든 대여정보 출력----" << endl;
    vector<shared_ptr<RentalInfo>> rentals = rentalManager.getAllRentals();
    for (auto rental : rentals) {
        setToDisplay(rental);
    }
    displayAllBuffer();
}
void BookService::displayRentalSearchBorrower() {
    cout << "대여자 이름을 입력하세요." << endl;
    string borrower = getInputString();
    vector<shared_ptr<RentalInfo>> rentals =
        rentalManager.getRentalsByBorrower(borrower);
    for (auto rental : rentals) {
        setToDisplay(rental);
    }
    displayAllBuffer();
}
void BookService::displayRentalSearchReturnDate() {
    cout << "기준 반납일을 입력하세요." << endl;
    cout << "----년도 입력----" << endl;
    int year = getInputInteger(2025, 2100);
    cout << "----월 입력----" << endl;
    int month = getInputInteger(1, 12);
    cout << "----일 입력----" << endl;
    int day = getInputInteger(1, 31);

    DateStruct returnDate{ year, month, day };

    vector<shared_ptr<RentalInfo>> rentals{
        rentalManager.getDelayedRentalsByReturnDate(returnDate) };
    for (auto rental : rentals) {
        setToDisplay(rental);
    }
    displayAllBuffer();
}

void BookService::displayAddBook() {
    cout << "추가할 책 제목과 작가를 입력하세요." << endl;
    cout << "----책 제목 입력----" << endl;
    string title = getInputString();
    cout << "----작가 입력----" << endl;
    string author = getInputString();
    bookManager.addBook(title, author);
}

void BookService::route() {
    bool isEnd = false;

    while (!isEnd) {
        displayMainMode();
        int mainMode = getInputInteger(1, 6);

        switch (MainMode(mainMode)) {
        case BOOK_SEARCH: {
            displayBookSearchMode();
            int bookSearchMode = getInputInteger(1, 3);

            switch (BookSearchMode(bookSearchMode)) {
            case ALL_BOOKS:
                displayBookSerachAllBooks();
                break;
            case TITLE:
                displayBookSearchTitle();
                break;
            case AUTHOR:
                displayBookSearchAuthor();
                break;
            }
            break;
        }
        case RENT:
            displayRent();
            break;
        case RETURN:
            displayReturn();
            break;
        case RENTAL_INFO_SEARCH: {
            displayRentalSearchMode();
            int rentalSearchMode = getInputInteger(1, 3);

            switch (RentalSearchMode(rentalSearchMode)) {
            case ALL_RENTALS:
                displayRentalSearchAllRentals();
                break;
            case BORROWER:
                displayRentalSearchBorrower();
                break;
            case RETURN_DATE:
                displayRentalSearchReturnDate();
                break;
            }
            break;
        }
        case ADD_BOOK:
            displayAddBook();
            break;
        case PROGRAM_END:
            isEnd = true;
            break;
        }
    }
}

int main() {

    BookManager bookManager;
    RentalManager rentalManager;
    BookService bookService(bookManager, rentalManager);

    bookManager.addBook("책1", "이승현");
    bookManager.addBook("책1", "이승현");
    bookManager.addBook("책1", "이승현");
    bookManager.addBook("책2", "이승현");
    bookManager.addBook("책2", "이승현");
    bookManager.addBook("책3", "승현");
    bookManager.addBook("책3", "승현");
    bookManager.addBook("책3", "승현");
    bookManager.addBook("책3", "승현");
    bookManager.addBook("책4", "승현");
    bookManager.addBook("책4", "승현");

    rentalManager.rentalBookById(
        1, RentalDTO("철수", "01012345678", DateStruct(2025, 1, 7)),
        bookManager);
    rentalManager.rentalBookById(
        2, RentalDTO("철수", "01012345678", DateStruct(2025, 1, 7)),
        bookManager);
    rentalManager.rentalBookById(
        2, RentalDTO("철수", "01012345678", DateStruct(2025, 1, 7)),
        bookManager);
    rentalManager.rentalBookByTitle(
        "책1", RentalDTO("영희", "01056781234", DateStruct(2025, 1, 6)),
        bookManager);
    rentalManager.rentalBookByTitle(
        "책1", RentalDTO("영희", "01056781234", DateStruct(2025, 1, 6)),
        bookManager);
    rentalManager.rentalBookByTitle(
        "책3", RentalDTO("영희", "01056781234", DateStruct(2025, 1, 6)),
        bookManager);

    bookService.route();

    return 0;
}

