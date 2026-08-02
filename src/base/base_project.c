/*  date = December 06th 2025 02:25 PM */


internal void 
BaseMainEntry(void (*entry)(U64 argument_count, char** arguments), U64 argument_count, char** arguments)
{
	// start profiler capture

	// Set QueryPerformanceCounter wallclock

	// init log arena

	// Init Clay

	// call os specific entry point
	entry(argument_count, arguments);

	// end profiler capture

}