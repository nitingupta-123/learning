class Solution {
  public:

  /*
    https://www.youtube.com/watch?v=AE39gJYuRog&list=PLgUwDviBIf0qUlt5H_kiKYaNSqJ81PMMY&index=8&t=1959s
    https://www.geeksforgeeks.org/problems/geeks-training/1
  */
  
    int recursiveAndMemoizedApproach(vector<vector<int>>& matrix,int day,int lastTask,vector<vector<int>>& dp){

        int maxi =0;
        if(day==0){
            for(int task=0;task<3;task++){
                if(task!=lastTask){
                    maxi = max(maxi, matrix[day][task]);
                }
            }
            return maxi;
        }

        if(dp[day][lastTask]!=-1){
            return dp[day][lastTask];
        }        
        for(int task=0;task<3;task++){
            if(task!=lastTask){
                maxi = max(maxi, matrix[day][task]+recursiveApproach(matrix,day-1,task,dp));
            }
        }

        return dp[day][lastTask]=maxi;

    }
    
    int tabulation(vector<vector<int>>&matrix){
        int n = matrix.size();
        vector<vector<int>>dp(n,vector<int>(4,-1));

        int day =0;
        
        for(int lastTask=0;lastTask<4;lastTask++){
            int maxi=0;
            for(int task=0;task<3;task++){
                if(task!=lastTask){
                    maxi = max(maxi, matrix[day][task]);
                }
            }
            dp[day][lastTask] = maxi;
        }
        
        for(day=1;day<n;day++){
            
            for(int lastTask=0;lastTask<4;lastTask++){
                int maxi=0;
                for(int task=0;task<3;task++){
                    if(task!=lastTask){
                        maxi = max(maxi, matrix[day][task]+dp[day-1][task]);
                    }
                }
                dp[day][lastTask]=maxi;
            }
        }
        
        return dp[n-1][3];
    }
    
    int tabulationSpaceOptimised(vector<vector<int>>&matrix){
        int n = matrix.size();
        // vector<vector<int>>dp(n,vector<int>(4,-1));
        vector<int>prevDay(4,0);
        vector<int>currDay(4,0);

        int day =0;
        
        for(int lastTask=0;lastTask<4;lastTask++){
            int maxi=0;
            for(int task=0;task<3;task++){
                if(task!=lastTask){
                    maxi = max(maxi, matrix[day][task]);
                }
            }
            prevDay[lastTask] = maxi;
        }
        
        for(day=1;day<n;day++){
            
            for(int lastTask=0;lastTask<4;lastTask++){
                int maxi=0;
                for(int task=0;task<3;task++){
                    if(task!=lastTask){
                        maxi = max(maxi, matrix[day][task]+ prevDay[task]);
                    }
                }
                currDay[lastTask]=maxi;
            }
            prevDay = currDay;
        }
        
        return currDay[3];
    }

    int maximumPoints(vector<vector<int>>& matrix) {
        // Code here
        
        return tabulationSpaceOptimised(matrix);
    }
};