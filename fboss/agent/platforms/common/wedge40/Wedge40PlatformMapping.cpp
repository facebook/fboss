/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/wedge40/Wedge40PlatformMapping.h"

namespace {
constexpr auto kJsonPlatformMappingStr = R"(
{
  "ports": {
    "1": {
        "mapping": {
          "id": 1,
          "name": "eth1/1/1",
          "controllingPort": 1,
          "pins": [
            {
              "a": {
                "chip": "WC9",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/1",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC9",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC9",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                2
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC9",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                2,
                3,
                4
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC9",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "WC9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC9",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                2,
                3,
                4
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC9",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "WC9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC9",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                2
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC9",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "2": {
        "mapping": {
          "id": 2,
          "name": "eth1/1/2",
          "controllingPort": 1,
          "pins": [
            {
              "a": {
                "chip": "WC9",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/1",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC9",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC9",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "3": {
        "mapping": {
          "id": 3,
          "name": "eth1/1/3",
          "controllingPort": 1,
          "pins": [
            {
              "a": {
                "chip": "WC9",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/1",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC9",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC9",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                4
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC9",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                4
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC9",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "4": {
        "mapping": {
          "id": 4,
          "name": "eth1/1/4",
          "controllingPort": 1,
          "pins": [
            {
              "a": {
                "chip": "WC9",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/1",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC9",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC9",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "5": {
        "mapping": {
          "id": 5,
          "name": "eth1/2/1",
          "controllingPort": 5,
          "pins": [
            {
              "a": {
                "chip": "WC8",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/2",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC8",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC8",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                6
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC8",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                6,
                7,
                8
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC8",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "WC8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC8",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                6,
                7,
                8
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC8",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "WC8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC8",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                6
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC8",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "6": {
        "mapping": {
          "id": 6,
          "name": "eth1/2/2",
          "controllingPort": 5,
          "pins": [
            {
              "a": {
                "chip": "WC8",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/2",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC8",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC8",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "7": {
        "mapping": {
          "id": 7,
          "name": "eth1/2/3",
          "controllingPort": 5,
          "pins": [
            {
              "a": {
                "chip": "WC8",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/2",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC8",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC8",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                8
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC8",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                8
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC8",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "8": {
        "mapping": {
          "id": 8,
          "name": "eth1/2/4",
          "controllingPort": 5,
          "pins": [
            {
              "a": {
                "chip": "WC8",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/2",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC8",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC8",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "9": {
        "mapping": {
          "id": 9,
          "name": "eth1/3/1",
          "controllingPort": 9,
          "pins": [
            {
              "a": {
                "chip": "WC11",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/3",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC11",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC11",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                10
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC11",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                10,
                11,
                12
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC11",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "WC11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC11",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                10,
                11,
                12
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC11",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "WC11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC11",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                10
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC11",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "10": {
        "mapping": {
          "id": 10,
          "name": "eth1/3/2",
          "controllingPort": 9,
          "pins": [
            {
              "a": {
                "chip": "WC11",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/3",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC11",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC11",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "11": {
        "mapping": {
          "id": 11,
          "name": "eth1/3/3",
          "controllingPort": 9,
          "pins": [
            {
              "a": {
                "chip": "WC11",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/3",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC11",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC11",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                12
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC11",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                12
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC11",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "12": {
        "mapping": {
          "id": 12,
          "name": "eth1/3/4",
          "controllingPort": 9,
          "pins": [
            {
              "a": {
                "chip": "WC11",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/3",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC11",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC11",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "13": {
        "mapping": {
          "id": 13,
          "name": "eth1/4/1",
          "controllingPort": 13,
          "pins": [
            {
              "a": {
                "chip": "WC10",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/4",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC10",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC10",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                14
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC10",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                14,
                15,
                16
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC10",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "WC10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC10",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                14,
                15,
                16
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC10",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "WC10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC10",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                14
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC10",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "14": {
        "mapping": {
          "id": 14,
          "name": "eth1/4/2",
          "controllingPort": 13,
          "pins": [
            {
              "a": {
                "chip": "WC10",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/4",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC10",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC10",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "15": {
        "mapping": {
          "id": 15,
          "name": "eth1/4/3",
          "controllingPort": 13,
          "pins": [
            {
              "a": {
                "chip": "WC10",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/4",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC10",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC10",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                16
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC10",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                16
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC10",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "16": {
        "mapping": {
          "id": 16,
          "name": "eth1/4/4",
          "controllingPort": 13,
          "pins": [
            {
              "a": {
                "chip": "WC10",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/4",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC10",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC10",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "17": {
        "mapping": {
          "id": 17,
          "name": "eth1/5/1",
          "controllingPort": 17,
          "pins": [
            {
              "a": {
                "chip": "WC13",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/5",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC13",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC13",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                18
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC13",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                18,
                19,
                20
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC13",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "WC13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC13",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                18,
                19,
                20
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC13",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "WC13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC13",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                18
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC13",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "18": {
        "mapping": {
          "id": 18,
          "name": "eth1/5/2",
          "controllingPort": 17,
          "pins": [
            {
              "a": {
                "chip": "WC13",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/5",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC13",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC13",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "19": {
        "mapping": {
          "id": 19,
          "name": "eth1/5/3",
          "controllingPort": 17,
          "pins": [
            {
              "a": {
                "chip": "WC13",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/5",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC13",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC13",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                20
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC13",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                20
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC13",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "20": {
        "mapping": {
          "id": 20,
          "name": "eth1/5/4",
          "controllingPort": 17,
          "pins": [
            {
              "a": {
                "chip": "WC13",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/5",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC13",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC13",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "21": {
        "mapping": {
          "id": 21,
          "name": "eth1/6/1",
          "controllingPort": 21,
          "pins": [
            {
              "a": {
                "chip": "WC12",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/6",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC12",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC12",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                22
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC12",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                22,
                23,
                24
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC12",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "WC12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC12",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                22,
                23,
                24
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC12",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "WC12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC12",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                22
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC12",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "22": {
        "mapping": {
          "id": 22,
          "name": "eth1/6/2",
          "controllingPort": 21,
          "pins": [
            {
              "a": {
                "chip": "WC12",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/6",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC12",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC12",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "23": {
        "mapping": {
          "id": 23,
          "name": "eth1/6/3",
          "controllingPort": 21,
          "pins": [
            {
              "a": {
                "chip": "WC12",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/6",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC12",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC12",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                24
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC12",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                24
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC12",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "24": {
        "mapping": {
          "id": 24,
          "name": "eth1/6/4",
          "controllingPort": 21,
          "pins": [
            {
              "a": {
                "chip": "WC12",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/6",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC12",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC12",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "25": {
        "mapping": {
          "id": 25,
          "name": "eth1/7/1",
          "controllingPort": 25,
          "pins": [
            {
              "a": {
                "chip": "WC15",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/7",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC15",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC15",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                26
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC15",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                26,
                27,
                28
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC15",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "WC15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC15",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                26,
                27,
                28
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC15",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "WC15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC15",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                26
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC15",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "26": {
        "mapping": {
          "id": 26,
          "name": "eth1/7/2",
          "controllingPort": 25,
          "pins": [
            {
              "a": {
                "chip": "WC15",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/7",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC15",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC15",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "27": {
        "mapping": {
          "id": 27,
          "name": "eth1/7/3",
          "controllingPort": 25,
          "pins": [
            {
              "a": {
                "chip": "WC15",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/7",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC15",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC15",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                28
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC15",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                28
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC15",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "28": {
        "mapping": {
          "id": 28,
          "name": "eth1/7/4",
          "controllingPort": 25,
          "pins": [
            {
              "a": {
                "chip": "WC15",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/7",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC15",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC15",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "29": {
        "mapping": {
          "id": 29,
          "name": "eth1/8/1",
          "controllingPort": 29,
          "pins": [
            {
              "a": {
                "chip": "WC14",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/8",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC14",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC14",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                30
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC14",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                30,
                31,
                32
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC14",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "WC14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC14",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                30,
                31,
                32
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC14",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "WC14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC14",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                30
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC14",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "30": {
        "mapping": {
          "id": 30,
          "name": "eth1/8/2",
          "controllingPort": 29,
          "pins": [
            {
              "a": {
                "chip": "WC14",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/8",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC14",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC14",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "31": {
        "mapping": {
          "id": 31,
          "name": "eth1/8/3",
          "controllingPort": 29,
          "pins": [
            {
              "a": {
                "chip": "WC14",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/8",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC14",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC14",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                32
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC14",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                32
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC14",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "32": {
        "mapping": {
          "id": 32,
          "name": "eth1/8/4",
          "controllingPort": 29,
          "pins": [
            {
              "a": {
                "chip": "WC14",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/8",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC14",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC14",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "33": {
        "mapping": {
          "id": 33,
          "name": "eth1/9/1",
          "controllingPort": 33,
          "pins": [
            {
              "a": {
                "chip": "WC17",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/9",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC17",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC17",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                34
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC17",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                34,
                35,
                36
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC17",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "WC17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC17",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                34,
                35,
                36
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC17",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "WC17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC17",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                34
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC17",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "34": {
        "mapping": {
          "id": 34,
          "name": "eth1/9/2",
          "controllingPort": 33,
          "pins": [
            {
              "a": {
                "chip": "WC17",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/9",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC17",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC17",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "35": {
        "mapping": {
          "id": 35,
          "name": "eth1/9/3",
          "controllingPort": 33,
          "pins": [
            {
              "a": {
                "chip": "WC17",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/9",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC17",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC17",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                36
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC17",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                36
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC17",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "36": {
        "mapping": {
          "id": 36,
          "name": "eth1/9/4",
          "controllingPort": 33,
          "pins": [
            {
              "a": {
                "chip": "WC17",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/9",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC17",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC17",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "37": {
        "mapping": {
          "id": 37,
          "name": "eth1/10/1",
          "controllingPort": 37,
          "pins": [
            {
              "a": {
                "chip": "WC16",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/10",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC16",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC16",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                38
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC16",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                38,
                39,
                40
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC16",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "WC16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC16",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                38,
                39,
                40
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC16",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "WC16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC16",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                38
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC16",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "38": {
        "mapping": {
          "id": 38,
          "name": "eth1/10/2",
          "controllingPort": 37,
          "pins": [
            {
              "a": {
                "chip": "WC16",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/10",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC16",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC16",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "39": {
        "mapping": {
          "id": 39,
          "name": "eth1/10/3",
          "controllingPort": 37,
          "pins": [
            {
              "a": {
                "chip": "WC16",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/10",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC16",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC16",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                40
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC16",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                40
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC16",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "40": {
        "mapping": {
          "id": 40,
          "name": "eth1/10/4",
          "controllingPort": 37,
          "pins": [
            {
              "a": {
                "chip": "WC16",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/10",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC16",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC16",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "41": {
        "mapping": {
          "id": 41,
          "name": "eth1/11/1",
          "controllingPort": 41,
          "pins": [
            {
              "a": {
                "chip": "WC19",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/11",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC19",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC19",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                42
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC19",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                42,
                43,
                44
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC19",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "WC19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC19",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                42,
                43,
                44
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC19",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "WC19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC19",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                42
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC19",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "42": {
        "mapping": {
          "id": 42,
          "name": "eth1/11/2",
          "controllingPort": 41,
          "pins": [
            {
              "a": {
                "chip": "WC19",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/11",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC19",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC19",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "43": {
        "mapping": {
          "id": 43,
          "name": "eth1/11/3",
          "controllingPort": 41,
          "pins": [
            {
              "a": {
                "chip": "WC19",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/11",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC19",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC19",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                44
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC19",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                44
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC19",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "44": {
        "mapping": {
          "id": 44,
          "name": "eth1/11/4",
          "controllingPort": 41,
          "pins": [
            {
              "a": {
                "chip": "WC19",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/11",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC19",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC19",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "45": {
        "mapping": {
          "id": 45,
          "name": "eth1/12/1",
          "controllingPort": 45,
          "pins": [
            {
              "a": {
                "chip": "WC18",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/12",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC18",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC18",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                46
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC18",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                46,
                47,
                48
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC18",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "WC18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC18",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                46,
                47,
                48
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC18",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "WC18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC18",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                46
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC18",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "46": {
        "mapping": {
          "id": 46,
          "name": "eth1/12/2",
          "controllingPort": 45,
          "pins": [
            {
              "a": {
                "chip": "WC18",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/12",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC18",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC18",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "47": {
        "mapping": {
          "id": 47,
          "name": "eth1/12/3",
          "controllingPort": 45,
          "pins": [
            {
              "a": {
                "chip": "WC18",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/12",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC18",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC18",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                48
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC18",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                48
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC18",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "48": {
        "mapping": {
          "id": 48,
          "name": "eth1/12/4",
          "controllingPort": 45,
          "pins": [
            {
              "a": {
                "chip": "WC18",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/12",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC18",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC18",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "49": {
        "mapping": {
          "id": 49,
          "name": "eth1/13/1",
          "controllingPort": 49,
          "pins": [
            {
              "a": {
                "chip": "WC21",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/13",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC21",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC21",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                50
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC21",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                50,
                51,
                52
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC21",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "WC21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC21",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                50,
                51,
                52
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC21",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "WC21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC21",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                50
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC21",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "50": {
        "mapping": {
          "id": 50,
          "name": "eth1/13/2",
          "controllingPort": 49,
          "pins": [
            {
              "a": {
                "chip": "WC21",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/13",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC21",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC21",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "51": {
        "mapping": {
          "id": 51,
          "name": "eth1/13/3",
          "controllingPort": 49,
          "pins": [
            {
              "a": {
                "chip": "WC21",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/13",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC21",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC21",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                52
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC21",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                52
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC21",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "52": {
        "mapping": {
          "id": 52,
          "name": "eth1/13/4",
          "controllingPort": 49,
          "pins": [
            {
              "a": {
                "chip": "WC21",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/13",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC21",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC21",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "53": {
        "mapping": {
          "id": 53,
          "name": "eth1/14/1",
          "controllingPort": 53,
          "pins": [
            {
              "a": {
                "chip": "WC20",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/14",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC20",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC20",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                54
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC20",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                54,
                55,
                56
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC20",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "WC20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC20",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                54,
                55,
                56
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC20",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "WC20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC20",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                54
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC20",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "54": {
        "mapping": {
          "id": 54,
          "name": "eth1/14/2",
          "controllingPort": 53,
          "pins": [
            {
              "a": {
                "chip": "WC20",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/14",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC20",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC20",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "55": {
        "mapping": {
          "id": 55,
          "name": "eth1/14/3",
          "controllingPort": 53,
          "pins": [
            {
              "a": {
                "chip": "WC20",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/14",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC20",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC20",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                56
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC20",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                56
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC20",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "56": {
        "mapping": {
          "id": 56,
          "name": "eth1/14/4",
          "controllingPort": 53,
          "pins": [
            {
              "a": {
                "chip": "WC20",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/14",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC20",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC20",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "57": {
        "mapping": {
          "id": 57,
          "name": "eth1/15/1",
          "controllingPort": 57,
          "pins": [
            {
              "a": {
                "chip": "WC23",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/15",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC23",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC23",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                58
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC23",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                58,
                59,
                60
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC23",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "WC23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC23",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                58,
                59,
                60
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC23",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "WC23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC23",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                58
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC23",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "58": {
        "mapping": {
          "id": 58,
          "name": "eth1/15/2",
          "controllingPort": 57,
          "pins": [
            {
              "a": {
                "chip": "WC23",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/15",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC23",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC23",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "59": {
        "mapping": {
          "id": 59,
          "name": "eth1/15/3",
          "controllingPort": 57,
          "pins": [
            {
              "a": {
                "chip": "WC23",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/15",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC23",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC23",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                60
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC23",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                60
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC23",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "60": {
        "mapping": {
          "id": 60,
          "name": "eth1/15/4",
          "controllingPort": 57,
          "pins": [
            {
              "a": {
                "chip": "WC23",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/15",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC23",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC23",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "61": {
        "mapping": {
          "id": 61,
          "name": "eth1/16/1",
          "controllingPort": 61,
          "pins": [
            {
              "a": {
                "chip": "WC22",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/16",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC22",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC22",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                62
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC22",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "17": {
              "subsumedPorts": [
                62,
                63,
                64
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC22",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "WC22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC22",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                62,
                63,
                64
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC22",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "WC22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC22",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                62
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "WC22",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "62": {
        "mapping": {
          "id": 62,
          "name": "eth1/16/2",
          "controllingPort": 61,
          "pins": [
            {
              "a": {
                "chip": "WC22",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/16",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC22",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC22",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "63": {
        "mapping": {
          "id": 63,
          "name": "eth1/16/3",
          "controllingPort": 61,
          "pins": [
            {
              "a": {
                "chip": "WC22",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/16",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC22",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC22",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "13": {
              "subsumedPorts": [
                64
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC22",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "29": {
              "subsumedPorts": [
                64
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "WC22",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "64": {
        "mapping": {
          "id": 64,
          "name": "eth1/16/4",
          "controllingPort": 61,
          "pins": [
            {
              "a": {
                "chip": "WC22",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/16",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "11": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC22",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "WC22",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    }
  },
  "chips": [
    {
      "name": "WC9",
      "type": 1,
      "physicalID": 9
    },
    {
      "name": "WC8",
      "type": 1,
      "physicalID": 8
    },
    {
      "name": "WC11",
      "type": 1,
      "physicalID": 11
    },
    {
      "name": "WC10",
      "type": 1,
      "physicalID": 10
    },
    {
      "name": "WC13",
      "type": 1,
      "physicalID": 13
    },
    {
      "name": "WC12",
      "type": 1,
      "physicalID": 12
    },
    {
      "name": "WC15",
      "type": 1,
      "physicalID": 15
    },
    {
      "name": "WC14",
      "type": 1,
      "physicalID": 14
    },
    {
      "name": "WC17",
      "type": 1,
      "physicalID": 17
    },
    {
      "name": "WC16",
      "type": 1,
      "physicalID": 16
    },
    {
      "name": "WC19",
      "type": 1,
      "physicalID": 19
    },
    {
      "name": "WC18",
      "type": 1,
      "physicalID": 18
    },
    {
      "name": "WC21",
      "type": 1,
      "physicalID": 21
    },
    {
      "name": "WC20",
      "type": 1,
      "physicalID": 20
    },
    {
      "name": "WC23",
      "type": 1,
      "physicalID": 23
    },
    {
      "name": "WC22",
      "type": 1,
      "physicalID": 22
    },
    {
      "name": "eth1/1",
      "type": 3,
      "physicalID": 0
    },
    {
      "name": "eth1/2",
      "type": 3,
      "physicalID": 1
    },
    {
      "name": "eth1/3",
      "type": 3,
      "physicalID": 2
    },
    {
      "name": "eth1/4",
      "type": 3,
      "physicalID": 3
    },
    {
      "name": "eth1/5",
      "type": 3,
      "physicalID": 4
    },
    {
      "name": "eth1/6",
      "type": 3,
      "physicalID": 5
    },
    {
      "name": "eth1/7",
      "type": 3,
      "physicalID": 6
    },
    {
      "name": "eth1/8",
      "type": 3,
      "physicalID": 7
    },
    {
      "name": "eth1/9",
      "type": 3,
      "physicalID": 8
    },
    {
      "name": "eth1/10",
      "type": 3,
      "physicalID": 9
    },
    {
      "name": "eth1/11",
      "type": 3,
      "physicalID": 10
    },
    {
      "name": "eth1/12",
      "type": 3,
      "physicalID": 11
    },
    {
      "name": "eth1/13",
      "type": 3,
      "physicalID": 12
    },
    {
      "name": "eth1/14",
      "type": 3,
      "physicalID": 13
    },
    {
      "name": "eth1/15",
      "type": 3,
      "physicalID": 14
    },
    {
      "name": "eth1/16",
      "type": 3,
      "physicalID": 15
    }
  ],
  "platformSupportedProfiles": [
    {
      "factor": {
        "profileID": 11
      },
      "profile": {
        "speed": 10000,
        "iphy": {
          "numLanes": 1,
          "modulation": 1,
          "fec": 1,
          "medium": 1,
          "interfaceMode": 10,
          "interfaceType": 10
        }
      }
    },
    {
      "factor": {
        "profileID": 12
      },
      "profile": {
        "speed": 10000,
        "iphy": {
          "numLanes": 1,
          "modulation": 1,
          "fec": 1,
          "medium": 2,
          "interfaceMode": 41,
          "interfaceType": 41
        }
      }
    },
    {
      "factor": {
        "profileID": 13
      },
      "profile": {
        "speed": 20000,
        "iphy": {
          "numLanes": 2,
          "modulation": 1,
          "fec": 1,
          "medium": 1,
          "interfaceMode": 11,
          "interfaceType": 11
        }
      }
    },
    {
      "factor": {
        "profileID": 29
      },
      "profile": {
        "speed": 20000,
        "iphy": {
          "numLanes": 2,
          "modulation": 1,
          "fec": 1,
          "medium": 2,
          "interfaceMode": 40,
          "interfaceType": 40
        }
      }
    },
    {
      "factor": {
        "profileID": 17
      },
      "profile": {
        "speed": 40000,
        "iphy": {
          "numLanes": 4,
          "modulation": 1,
          "fec": 1,
          "medium": 1,
          "interfaceMode": 12,
          "interfaceType": 12
        }
      }
    },
    {
      "factor": {
        "profileID": 18
      },
      "profile": {
        "speed": 40000,
        "iphy": {
          "numLanes": 4,
          "modulation": 1,
          "fec": 1,
          "medium": 2,
          "interfaceMode": 40,
          "interfaceType": 40
        }
      }
    }
  ]
}
)";
} // namespace

namespace facebook {
namespace fboss {
Wedge40PlatformMapping::Wedge40PlatformMapping()
    : PlatformMapping(kJsonPlatformMappingStr) {}

Wedge40PlatformMapping::Wedge40PlatformMapping(
    const std::string& platformMappingStr)
    : PlatformMapping(platformMappingStr) {}

} // namespace fboss
} // namespace facebook
