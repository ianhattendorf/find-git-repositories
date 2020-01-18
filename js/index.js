const { findGitRepos } = require('bindings')('findGitRepos');

const result = findGitRepos("C:\\my\\repos", (arg1) => console.log('PROGRESS:', arg1));
console.log({ result });
result.then(console.log).catch(console.error);

module.exports = findGitRepos;