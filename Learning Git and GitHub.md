# **Learning Git and GitHub**

##### 

###### Level 1: Git Basics



1\. We use windows powershell (Powershell = cmd + some extra features)

2\. **cd** **'folder path'** to get to where we want to work

3\. **ls**: Lists all files inside the folder

4\. **git init**: to start using git in that folder
5. **git status**: to tell us any changes since last commit
6. **git add** filename ..... git add folder/ .... git add .: To pick changes to go the next commit
7. **git commit -m "**Message 1**"** : to create a commit with a mssg attached
8. **git commit -m "**message**" --amend** : Update previous commit instead of creating new one
9. **git log** : to get commit history
10. To visualize all these git thingies, u can use the git tab in vscode...In there u will see changes (working area) which u can add to staging area (changes there will go to next commit)
11. **Git reset** ...: Literally the opposite of git add command (acts the same way as it but removes changes from the staging area to the working area)
12. **git checkout --** filename/folder/. : Undo changes in the working area
13. **git checkout CommitID** : To go to previous commit   ------> (This will make ur code go to that version of code ...if u did git log, it will show that commit and the commits before it so in order to show all commit history just better use **git log --all**)

14\. Press q to exit the git log if needed
15. Imagine this Scenario: U created commits named "Version 1", "Version 2" \& "Version 3" sequentially and then u used git checkout command to go back to version 1 and then u did some changes and then u used the git commit and named it version 1 updated... what will happen in this scenario is that a branch is made from version 1 instead of a new commit added on top of version 3... look @ *Visual Explanation1* 
  A. **git log --all --graph** : Show branching visually in the command line
  B. Branch name: master/main...etc ----> can be used in git checkout instead of long id + it always points to last commit in the branch
  C. To solve the problem: git checkout CommitID <file|folder> ... and then commit -m "Version 1 updated"

