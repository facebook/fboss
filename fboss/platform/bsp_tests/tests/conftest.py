def pytest_configure(config):
    config.addinivalue_line("markers", "stress: mark test as a stress test")
