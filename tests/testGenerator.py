import random

if __name__ == "__main__":
    
    qtNumbers = [20, 50, 100, 1000, 2000, 3000, 5000, 10000]
    
    for numQt in qtNumbers:
        print(f"Generating {numQt} random numbers")
        numbersGeneratedFileName = "./testCases/test"+str(numQt)+".txt"
        expectedSumFileName = "./resultsExpected/test"+str(numQt)+"SumExpected.txt"
        
        with open(expectedSumFileName,"w") as sumExpectedFile:
            sum = 0
            with open(numbersGeneratedFileName,"w") as numbersFile:

                numbersFile.write("all\n")
                numbersFile.write(f"{numQt}\n")

                for numCount in range(numQt):
                    randomNumber = random.random() * 100
                    randomNumber = round(randomNumber, 2)
                    sum += randomNumber
                    numbersFile.write(f"{randomNumber} ")

                sumExpectedFile.write(str(round(sum,2)))