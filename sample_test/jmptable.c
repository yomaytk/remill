#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// 関数の定義
void function1() {
    printf("Function 1 called\n");
}

void function2() {
    printf("Function 2 called\n");
}

void function3() {
    printf("Function 3 called\n");
}

int main() {
    // 乱数の初期化
    srand(time(NULL));

    // 関数ポインタの配列（ジャンプテーブル）を定義
    void (*jump_table[3])() = {function1, function2, function3};

    // 1から3の間の乱数を生成
    int choice = (rand() % 3) + 1;

    printf("Randomly selected number: %d\n", choice);

    // ジャンプテーブルを使って関数を呼び出し
    jump_table[choice - 1]();

    return 0;
}

