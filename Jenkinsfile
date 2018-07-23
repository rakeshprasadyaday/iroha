properties([
  parameters([
    booleanParam(defaultValue: false, description: 'Build `iroha`', name: 'iroha'),
    booleanParam(defaultValue: false, description: 'Run iroha tests?', name: 'irohaTests'),
    booleanParam(defaultValue: false, description: 'Collect iroha coverage?', name: 'irohaCoverage'),
    booleanParam(defaultValue: true, description: 'Build `bindings`', name: 'irohaBindings'),
    booleanParam(defaultValue: false, name: 'amd64'),
    booleanParam(defaultValue: true, name: 'arm64'),
    booleanParam(defaultValue: false, name: 'armhf'),
    booleanParam(defaultValue: false, name: 'ubuntu_xenial'),
    booleanParam(defaultValue: false, name: 'ubuntu_bionic'),
    booleanParam(defaultValue: true, name: 'debian_stretch'),
    booleanParam(defaultValue: false, name: 'macos'),
    booleanParam(defaultValue: false, name: 'windows'),
    choice(choices: 'Debug\nRelease', description: 'Iroha build type', name: 'irohaBuildType'),
    booleanParam(defaultValue: true, description: 'Build Java bindings', name: 'JavaBindings'),
    choice(choices: 'Release\nDebug', description: 'Java bindings build type', name: 'JBBuildType'),
    string(defaultValue: 'jp.co.soramitsu.iroha', description: 'Java bindings package name', name: 'JBPackageName'),
    booleanParam(defaultValue: false, description: 'Build Python bindings', name: 'PythonBindings'),
    choice(choices: 'Release\nDebug', description: 'Python bindings build type', name: 'PBBuildType'),
    choice(choices: 'python3\npython2', description: 'Python bindings version', name: 'PBVersion'),
    booleanParam(defaultValue: false, description: 'Build Android bindings', name: 'AndroidBindings'),
    choice(choices: '26\n25\n24\n23\n22\n21\n20\n19\n18\n17\n16\n15\n14', description: 'Android Bindings ABI Version', name: 'ABABIVersion'),
    choice(choices: 'Release\nDebug', description: 'Android bindings build type', name: 'ABBuildType'),
    choice(choices: 'arm64-v8a\narmeabi-v7a\narmeabi\nx86_64\nx86', description: 'Android bindings platform', name: 'ABPlatform'),
    booleanParam(defaultValue: false, description: 'Build docs', name: 'Doxygen'),
    string(defaultValue: '8', description: 'How much parallelism should we exploit. "4" is optimal for machines with modest amount of memory and at least 4 cores', name: 'PARALLELISM')
  ]),
  buildDiscarder(logRotator(numToKeepStr: '20'))
])

def environmentList = []
def environment = [:]
def tasks = [:]
def jobs = []
def agentLabels = ['amd64-agent': 'ec2-fleet',
                   'arm64-agent': 'armv8-cross',
                   'armhf-agent': 'armv7-cross',
                   'mac-agent': 'mac',
                   'windows-agent': 'win']
def agentsMap =
  [
  'build':[
    (agentLabels['amd64-agent']):
    [
      ['amd64', 'ubuntu_xenial'],
      ['amd64', 'ubuntu_bionic'],
      ['amd64', 'debian_stretch'],
      ['arm64', 'ubuntu_xenial'],
      ['arm64', 'ubuntu_bionic'],
      ['arm64', 'debian_stretch'],
      ['armhf', 'ubuntu_xenial'],
      ['armhf', 'ubuntu_bionic'],
      ['armhf', 'debian_stretch']],
    (agentLabels['mac-agent']):
    [
      ['amd64', 'macos']],
    (agentLabels['windows-agent']):
    [
      ['amd64', 'windows']
    ]],
  'test':[
    (agentLabels['amd64-agent']):
    [
      ['amd64', 'ubuntu_xenial'],
      ['amd64', 'ubuntu_bionic'],
      ['amd64', 'debian_stretch']],
    (agentLabels['arm64-agent']):
    [
      ['arm64', 'ubuntu_xenial'],
      ['arm64', 'ubuntu_bionic'],
      ['arm64', 'debian_stretch']],
    (agentLabels['armhf-agent']):
    [
      ['armhf', 'ubuntu_xenial'],
      ['armhf', 'ubuntu_bionic'],
      ['armhf', 'debian_stretch']],
    (agentLabels['mac-agent']):
    [
      ['amd64', 'macos']],
    (agentLabels['windows-agent']):
    [
      ['amd64', 'windows']]
    ]]


def userInputArchOsTuples() {
  combinationsTuples = []
  m = ['arch': ['amd64': params.amd64, 'arm64': params.arm64, 'armhf': params.armhf],
       'os'  : ['ubuntu_xenial': params.ubuntu_xenial,
                'ubuntu_bionic': params.ubuntu_bionic,
                'debian_stretch': params.debian_stretch,
                'macos': params.macos,
                'windows': params.windows]]
  mArch = m['arch'].findAll { it.value == true }.collect { it.key }
  mOs = m['os'].findAll { it.value == true }.collect { it.key }
  combinationsList = GroovyCollections.combinations([mArch, mOs])
  combinationsList.each { it ->
    combinationsTuples.add([it[0], it[1]])
  }
  return combinationsTuples
}

node('master') {
  def scmVars = checkout scm
  def iroha = load ".jenkinsci/iroha.groovy"
  def bindings = load ".jenkinsci/bindings.groovy"
  environment = [
    "CCACHE_DIR": "/opt/.ccache",
    "DOCKER_REGISTRY_BASENAME": "hyperledger/iroha",
    "IROHA_NETWORK": "iroha-0${scmVars.CHANGE_ID}-${scmVars.GIT_COMMIT}-${env.BUILD_NUMBER}",
    "IROHA_POSTGRES_HOST": "pg-0${scmVars.CHANGE_ID}-${scmVars.GIT_COMMIT}-${env.BUILD_NUMBER}",
    "IROHA_POSTGRES_USER": "pguser${scmVars.GIT_COMMIT}",
    "IROHA_POSTGRES_PASSWORD": "${scmVars.GIT_COMMIT}",
    "IROHA_POSTGRES_PORT": "5432",
    "WS_BASE_DIR": "/var/jenkins/workspace",
    "GIT_RAW_BASE_URL": "https://raw.githubusercontent.com/hyperledger/iroha",
    "DOCKER_REGISTRY_CREDENTIALS_ID": 'docker-hub-credentials',
    "JAVA_HOME": "/usr/lib/jvm/java-8-oracle"
  ]
  environment.each { e ->
    environmentList.add("${e.key}=${e.value}")
  }
  // build Iroha binaries
  if(params.iroha) {
    println("Build Iroha")
    builders = agentsMap['build'].each { k, v -> v.retainAll(userInputArchOsTuples() as Object[])}
    builders.each { agent, platform ->
      for(t in platform) {
        if(t.size() > 0) {
          def platformArch = t[0]
          def platformOS = t[1]
          def dockerImage = ''
          // windows and mac are built on a host, not in docker
          if(['ubuntu_xenial', 'ubuntu_bionic', 'debian_stretch'].contains(platformOS)) {
            platformOS = platformOS.replaceAll('_', '-')
            dockerImage = "${environment['DOCKER_REGISTRY_BASENAME']}:crossbuild-${platformOS}-${platformArch}"
          }
          jobs.add([iroha.buildSteps(agent, platformArch, platformOS, params.irohaBuildType,
            params.irohaCoverage, environmentList, dockerImage)])
        }
      }
    }
    // run tests if required
    if(params.irohaTests) {
      testers = agentsMap['test'].each { k, v -> v.retainAll(userInputArchOsTuples() as Object[])}
      println("testers is: ${testers}")
      testers.each { agent, platform ->
        for(t in platform) {
          if(t.size() > 0) {
            def platformArch = t[0]
            def platformOS = t[1]
            def dockerImage = ''
            // windows and mac are built on a host, not in docker
            if(['ubuntu_xenial', 'ubuntu_bionic', 'debian_stretch'].contains(platformOS)) {
              //dockerImage = "${environment['DOCKER_REGISTRY_BASENAME']}:crossbuild-${os}-${arch}"
              dockerImage = 'local/linux-test-env'
            }
            job = iroha.testSteps(agent, platformArch, platformOS,
                            params.irohaCoverage, environmentList, dockerImage)
            jobs.collect { it.add(job) }
          }
        }
      }
    }
  }
  if(params.irohaBindings) {
    builders = agentsMap['build'].each { k, v -> v.retainAll(userInputArchOsTuples() as Object[])}
    builders.each { agent, platform ->
      for(t in platform) {
        if(t.size() > 0) {
          def platformArch = t[0]
          def platformOS = t[1]
          def dockerImage = ''
          // windows and mac are built on a host, not in docker
          if(['ubuntu_xenial', 'ubuntu_bionic', 'debian_stretch'].contains(platformOS)) {
            platformOS = platformOS.replaceAll('_', '-')
            dockerImage = "${environment['DOCKER_REGISTRY_BASENAME']}:crossbuild-${platformOS}-${platformArch}"
            platformOS = 'linux'
          }
          if(params.JavaBindings) {
            jobs.add([bindings.buildSteps(agent, platformArch, platformOS, params.JBBuildType,
              params.JBPackageName, 'java', environmentList, dockerImage)])
          }
        }
      }
    }
  }
}

if(jobs) {
  for(int i=0; i<jobs.size(); i++) {
    def job = jobs[i]
    tasks["${i}"] = {
      job.each { it() }
    }
  }
  stage('Build & Test') {
    parallel tasks
  }
}

// build bindings
// build docs
