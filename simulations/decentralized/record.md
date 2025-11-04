[Test 1 - HESL]

Configuration:

    [Config Normal]    # Experiment without network failure
    extends = Uploading

    sim-time-limit = 900s
    **.rescheduleAll = true
    *.numUe = 80 	# 80
    **.srsDistanceCheck = true  # disable the checking the distance for SRS transmission, for testing purpose
    **.countExeTime = true

    # **.fairFactor = 1    # the fairness factor for the scheduler
    # *.ue[*].app[*].dlScale = 1    # the scale for the deadline of the app, default is 1.0. dl = dl / dlScale
    # **.serverExeScale = 3.0   # the scale for the server execution time for database, default is 1.0

    *.ue[*].numApps = ${appCount=3, 4}
    **.scheduleScheme = "${scheme=Greedy, FastSA, GameTheory, GraphMatch}"	# "Greedy", "FastSA", "GameTheory", "GraphMatch"
    **.scheduleInterval = ${interval=5, 10, 15}s	# ${interval=5, 10, 15, 20}s
    **.pilotMode = "MAX_CQI"   # "${pilot=MAX_CQI, MEDIAN_CQI, MIN_CQI}"
    # **.rescheduleAll = ${rsAll=true, false}	# true, false
    # **.countExeTime = ${countTime=true, false}	# whether to count the execution time of schedule scheme, default is true

    # Statistics 
    output-scalar-file = ${resultdir}/${configname}/${scheme}-interval-${interval}-appCount-${appCount}.sca
    output-vector-file = ${resultdir}/${configname}/${scheme}-interval-${interval}-appCount-${appCount}.vec


