var solution = new Solution('Exercise6');
var project = new Project('Exercise6');

project.addFile('Sources/**');
project.setDebugDir('Deployment');

project.addSubProject(Solution.createProject('Kore'));

solution.addProject(project)

return solution;
