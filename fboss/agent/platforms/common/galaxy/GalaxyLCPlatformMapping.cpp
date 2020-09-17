/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/galaxy/GalaxyLCPlatformMapping.h"

#include "fboss/agent/FbossError.h"

#include <folly/FileUtil.h>
#include <folly/json.h>

#include <boost/algorithm/string.hpp>
#include <re2/re2.h>

DEFINE_string(
    netwhoami,
    "/etc/netwhoami.json",
    "The path to the local JSON file");

namespace {
constexpr auto kJsonPlatformMappingStr = R"(
{
  "ports": {
    "1": {
        "mapping": {
          "id": 1,
          "name": "eth101/13/1",
          "controllingPort": 1,
          "pins": [
            {
              "a": {
                "chip": "FC0",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth101/13",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/13",
                      "lane": 0
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
                      "chip": "FC0",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/13",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/13",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                2,
                3,
                4
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/13",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/13",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/13",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                2
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/13",
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
          "name": "eth101/13/2",
          "controllingPort": 1,
          "pins": [
            {
              "a": {
                "chip": "FC0",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth101/13",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/13",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/13",
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
          "name": "eth101/13/3",
          "controllingPort": 1,
          "pins": [
            {
              "a": {
                "chip": "FC0",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth101/13",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/13",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/13",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                4
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/13",
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
          "name": "eth101/13/4",
          "controllingPort": 1,
          "pins": [
            {
              "a": {
                "chip": "FC0",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth101/13",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/13",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/13",
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
          "name": "eth101/14/1",
          "controllingPort": 5,
          "pins": [
            {
              "a": {
                "chip": "FC1",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth101/14",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/14",
                      "lane": 0
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
                      "chip": "FC1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/14",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/14",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                6,
                7,
                8
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/14",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/14",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/14",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                6
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/14",
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
          "name": "eth101/14/2",
          "controllingPort": 5,
          "pins": [
            {
              "a": {
                "chip": "FC1",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth101/14",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/14",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/14",
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
          "name": "eth101/14/3",
          "controllingPort": 5,
          "pins": [
            {
              "a": {
                "chip": "FC1",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth101/14",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/14",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/14",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                8
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/14",
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
          "name": "eth101/14/4",
          "controllingPort": 5,
          "pins": [
            {
              "a": {
                "chip": "FC1",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth101/14",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/14",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/14",
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
          "name": "eth101/15/1",
          "controllingPort": 9,
          "pins": [
            {
              "a": {
                "chip": "FC2",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth101/15",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/15",
                      "lane": 0
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
                      "chip": "FC2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/15",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/15",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                10,
                11,
                12
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/15",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/15",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/15",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                10
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/15",
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
          "name": "eth101/15/2",
          "controllingPort": 9,
          "pins": [
            {
              "a": {
                "chip": "FC2",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth101/15",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/15",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/15",
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
          "name": "eth101/15/3",
          "controllingPort": 9,
          "pins": [
            {
              "a": {
                "chip": "FC2",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth101/15",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/15",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/15",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                12
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/15",
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
          "name": "eth101/15/4",
          "controllingPort": 9,
          "pins": [
            {
              "a": {
                "chip": "FC2",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth101/15",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/15",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/15",
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
          "name": "eth101/16/1",
          "controllingPort": 13,
          "pins": [
            {
              "a": {
                "chip": "FC3",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth101/16",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/16",
                      "lane": 0
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
                      "chip": "FC3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/16",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/16",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                14,
                15,
                16
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/16",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/16",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/16",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                14
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/16",
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
          "name": "eth101/16/2",
          "controllingPort": 13,
          "pins": [
            {
              "a": {
                "chip": "FC3",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth101/16",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/16",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/16",
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
          "name": "eth101/16/3",
          "controllingPort": 13,
          "pins": [
            {
              "a": {
                "chip": "FC3",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth101/16",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/16",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/16",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                16
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/16",
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
          "name": "eth101/16/4",
          "controllingPort": 13,
          "pins": [
            {
              "a": {
                "chip": "FC3",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth101/16",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/16",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/16",
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
          "name": "fab101/29/1",
          "controllingPort": 17,
          "pins": [
            {
              "a": {
                "chip": "FC4",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                18
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                18,
                19,
                20
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 3
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
          "name": "fab101/29/2",
          "controllingPort": 17,
          "pins": [
            {
              "a": {
                "chip": "FC4",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
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
          "name": "fab101/29/3",
          "controllingPort": 17,
          "pins": [
            {
              "a": {
                "chip": "FC4",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                20
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC4",
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
          "name": "fab101/29/4",
          "controllingPort": 17,
          "pins": [
            {
              "a": {
                "chip": "FC4",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
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
          "name": "fab101/30/1",
          "controllingPort": 21,
          "pins": [
            {
              "a": {
                "chip": "FC5",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                22
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                22,
                23,
                24
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 3
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
          "name": "fab101/30/2",
          "controllingPort": 21,
          "pins": [
            {
              "a": {
                "chip": "FC5",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
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
          "name": "fab101/30/3",
          "controllingPort": 21,
          "pins": [
            {
              "a": {
                "chip": "FC5",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                24
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC5",
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
          "name": "fab101/30/4",
          "controllingPort": 21,
          "pins": [
            {
              "a": {
                "chip": "FC5",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
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
          "name": "fab101/31/1",
          "controllingPort": 25,
          "pins": [
            {
              "a": {
                "chip": "FC6",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                26
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                26,
                27,
                28
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 3
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
          "name": "fab101/31/2",
          "controllingPort": 25,
          "pins": [
            {
              "a": {
                "chip": "FC6",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
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
          "name": "fab101/31/3",
          "controllingPort": 25,
          "pins": [
            {
              "a": {
                "chip": "FC6",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                28
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC6",
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
          "name": "fab101/31/4",
          "controllingPort": 25,
          "pins": [
            {
              "a": {
                "chip": "FC6",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
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
          "name": "fab101/32/1",
          "controllingPort": 29,
          "pins": [
            {
              "a": {
                "chip": "FC7",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                30
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                30,
                31,
                32
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 3
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
          "name": "fab101/32/2",
          "controllingPort": 29,
          "pins": [
            {
              "a": {
                "chip": "FC7",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
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
          "name": "fab101/32/3",
          "controllingPort": 29,
          "pins": [
            {
              "a": {
                "chip": "FC7",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                32
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC7",
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
          "name": "fab101/32/4",
          "controllingPort": 29,
          "pins": [
            {
              "a": {
                "chip": "FC7",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 3
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
          "name": "fab101/25/1",
          "controllingPort": 34,
          "pins": [
            {
              "a": {
                "chip": "FC8",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                35
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                35,
                36,
                37
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 3
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
          "name": "fab101/25/2",
          "controllingPort": 34,
          "pins": [
            {
              "a": {
                "chip": "FC8",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 1
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
          "name": "fab101/25/3",
          "controllingPort": 34,
          "pins": [
            {
              "a": {
                "chip": "FC8",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                37
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC8",
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
          "name": "fab101/25/4",
          "controllingPort": 34,
          "pins": [
            {
              "a": {
                "chip": "FC8",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 3
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
          "name": "fab101/26/1",
          "controllingPort": 38,
          "pins": [
            {
              "a": {
                "chip": "FC9",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                39
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                39,
                40,
                41
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 3
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
          "name": "fab101/26/2",
          "controllingPort": 38,
          "pins": [
            {
              "a": {
                "chip": "FC9",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 1
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
          "name": "fab101/26/3",
          "controllingPort": 38,
          "pins": [
            {
              "a": {
                "chip": "FC9",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                41
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC9",
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
          "name": "fab101/26/4",
          "controllingPort": 38,
          "pins": [
            {
              "a": {
                "chip": "FC9",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 3
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
          "name": "fab101/27/1",
          "controllingPort": 42,
          "pins": [
            {
              "a": {
                "chip": "FC10",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                43
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                43,
                44,
                45
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 3
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
          "name": "fab101/27/2",
          "controllingPort": 42,
          "pins": [
            {
              "a": {
                "chip": "FC10",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 1
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
          "name": "fab101/27/3",
          "controllingPort": 42,
          "pins": [
            {
              "a": {
                "chip": "FC10",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                45
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC10",
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
          "name": "fab101/27/4",
          "controllingPort": 42,
          "pins": [
            {
              "a": {
                "chip": "FC10",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 3
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
          "name": "fab101/28/1",
          "controllingPort": 46,
          "pins": [
            {
              "a": {
                "chip": "FC11",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                47
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                47,
                48,
                49
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 3
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
          "name": "fab101/28/2",
          "controllingPort": 46,
          "pins": [
            {
              "a": {
                "chip": "FC11",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 1
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
          "name": "fab101/28/3",
          "controllingPort": 46,
          "pins": [
            {
              "a": {
                "chip": "FC11",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                49
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC11",
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
          "name": "fab101/28/4",
          "controllingPort": 46,
          "pins": [
            {
              "a": {
                "chip": "FC11",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 3
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
          "name": "fab101/21/1",
          "controllingPort": 50,
          "pins": [
            {
              "a": {
                "chip": "FC12",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                51
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                51,
                52,
                53
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 3
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
          "name": "fab101/21/2",
          "controllingPort": 50,
          "pins": [
            {
              "a": {
                "chip": "FC12",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 1
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
          "name": "fab101/21/3",
          "controllingPort": 50,
          "pins": [
            {
              "a": {
                "chip": "FC12",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                53
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC12",
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
          "name": "fab101/21/4",
          "controllingPort": 50,
          "pins": [
            {
              "a": {
                "chip": "FC12",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 3
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
          "name": "fab101/22/1",
          "controllingPort": 54,
          "pins": [
            {
              "a": {
                "chip": "FC13",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                55
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                55,
                56,
                57
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 3
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
          "name": "fab101/22/2",
          "controllingPort": 54,
          "pins": [
            {
              "a": {
                "chip": "FC13",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 1
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
          "name": "fab101/22/3",
          "controllingPort": 54,
          "pins": [
            {
              "a": {
                "chip": "FC13",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                57
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC13",
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
          "name": "fab101/22/4",
          "controllingPort": 54,
          "pins": [
            {
              "a": {
                "chip": "FC13",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 3
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
          "name": "fab101/23/1",
          "controllingPort": 58,
          "pins": [
            {
              "a": {
                "chip": "FC14",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                59
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                59,
                60,
                61
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 3
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
          "name": "fab101/23/2",
          "controllingPort": 58,
          "pins": [
            {
              "a": {
                "chip": "FC14",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 1
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
          "name": "fab101/23/3",
          "controllingPort": 58,
          "pins": [
            {
              "a": {
                "chip": "FC14",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                61
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC14",
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
          "name": "fab101/23/4",
          "controllingPort": 58,
          "pins": [
            {
              "a": {
                "chip": "FC14",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 3
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
          "name": "fab101/24/1",
          "controllingPort": 62,
          "pins": [
            {
              "a": {
                "chip": "FC15",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                63
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                63,
                64,
                65
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 3
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
          "name": "fab101/24/2",
          "controllingPort": 62,
          "pins": [
            {
              "a": {
                "chip": "FC15",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 1
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
          "name": "fab101/24/3",
          "controllingPort": 62,
          "pins": [
            {
              "a": {
                "chip": "FC15",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                65
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "65": {
        "mapping": {
          "id": 65,
          "name": "fab101/24/4",
          "controllingPort": 62,
          "pins": [
            {
              "a": {
                "chip": "FC15",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "68": {
        "mapping": {
          "id": 68,
          "name": "fab101/17/1",
          "controllingPort": 68,
          "pins": [
            {
              "a": {
                "chip": "FC16",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                69
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                69,
                70,
                71
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "69": {
        "mapping": {
          "id": 69,
          "name": "fab101/17/2",
          "controllingPort": 68,
          "pins": [
            {
              "a": {
                "chip": "FC16",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "70": {
        "mapping": {
          "id": 70,
          "name": "fab101/17/3",
          "controllingPort": 68,
          "pins": [
            {
              "a": {
                "chip": "FC16",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                71
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "71": {
        "mapping": {
          "id": 71,
          "name": "fab101/17/4",
          "controllingPort": 68,
          "pins": [
            {
              "a": {
                "chip": "FC16",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "72": {
        "mapping": {
          "id": 72,
          "name": "fab101/18/1",
          "controllingPort": 72,
          "pins": [
            {
              "a": {
                "chip": "FC17",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                73
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                73,
                74,
                75
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "73": {
        "mapping": {
          "id": 73,
          "name": "fab101/18/2",
          "controllingPort": 72,
          "pins": [
            {
              "a": {
                "chip": "FC17",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "74": {
        "mapping": {
          "id": 74,
          "name": "fab101/18/3",
          "controllingPort": 72,
          "pins": [
            {
              "a": {
                "chip": "FC17",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                75
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "75": {
        "mapping": {
          "id": 75,
          "name": "fab101/18/4",
          "controllingPort": 72,
          "pins": [
            {
              "a": {
                "chip": "FC17",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "76": {
        "mapping": {
          "id": 76,
          "name": "fab101/19/1",
          "controllingPort": 76,
          "pins": [
            {
              "a": {
                "chip": "FC18",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                77
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                77,
                78,
                79
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "77": {
        "mapping": {
          "id": 77,
          "name": "fab101/19/2",
          "controllingPort": 76,
          "pins": [
            {
              "a": {
                "chip": "FC18",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "78": {
        "mapping": {
          "id": 78,
          "name": "fab101/19/3",
          "controllingPort": 76,
          "pins": [
            {
              "a": {
                "chip": "FC18",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                79
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "79": {
        "mapping": {
          "id": 79,
          "name": "fab101/19/4",
          "controllingPort": 76,
          "pins": [
            {
              "a": {
                "chip": "FC18",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "80": {
        "mapping": {
          "id": 80,
          "name": "fab101/20/1",
          "controllingPort": 80,
          "pins": [
            {
              "a": {
                "chip": "FC19",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                81
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                81,
                82,
                83
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "81": {
        "mapping": {
          "id": 81,
          "name": "fab101/20/2",
          "controllingPort": 80,
          "pins": [
            {
              "a": {
                "chip": "FC19",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "82": {
        "mapping": {
          "id": 82,
          "name": "fab101/20/3",
          "controllingPort": 80,
          "pins": [
            {
              "a": {
                "chip": "FC19",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                83
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "83": {
        "mapping": {
          "id": 83,
          "name": "fab101/20/4",
          "controllingPort": 80,
          "pins": [
            {
              "a": {
                "chip": "FC19",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "84": {
        "mapping": {
          "id": 84,
          "name": "eth101/1/1",
          "controllingPort": 84,
          "pins": [
            {
              "a": {
                "chip": "FC20",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth101/1",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/1",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                85,
                86,
                87
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/1",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/1",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                85,
                86,
                87
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/1",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/1",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/1",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                85
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/1",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "85": {
        "mapping": {
          "id": 85,
          "name": "eth101/1/2",
          "controllingPort": 84,
          "pins": [
            {
              "a": {
                "chip": "FC20",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth101/1",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/1",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/1",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "86": {
        "mapping": {
          "id": 86,
          "name": "eth101/1/3",
          "controllingPort": 84,
          "pins": [
            {
              "a": {
                "chip": "FC20",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth101/1",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/1",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/1",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                87
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/1",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "87": {
        "mapping": {
          "id": 87,
          "name": "eth101/1/4",
          "controllingPort": 84,
          "pins": [
            {
              "a": {
                "chip": "FC20",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth101/1",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/1",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/1",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "88": {
        "mapping": {
          "id": 88,
          "name": "eth101/2/1",
          "controllingPort": 88,
          "pins": [
            {
              "a": {
                "chip": "FC21",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth101/2",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/2",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                89,
                90,
                91
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/2",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/2",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                89,
                90,
                91
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/2",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/2",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/2",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                89
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/2",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "89": {
        "mapping": {
          "id": 89,
          "name": "eth101/2/2",
          "controllingPort": 88,
          "pins": [
            {
              "a": {
                "chip": "FC21",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth101/2",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/2",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/2",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "90": {
        "mapping": {
          "id": 90,
          "name": "eth101/2/3",
          "controllingPort": 88,
          "pins": [
            {
              "a": {
                "chip": "FC21",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth101/2",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/2",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/2",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                91
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/2",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "91": {
        "mapping": {
          "id": 91,
          "name": "eth101/2/4",
          "controllingPort": 88,
          "pins": [
            {
              "a": {
                "chip": "FC21",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth101/2",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/2",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/2",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "92": {
        "mapping": {
          "id": 92,
          "name": "eth101/3/1",
          "controllingPort": 92,
          "pins": [
            {
              "a": {
                "chip": "FC22",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth101/3",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/3",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                93,
                94,
                95
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/3",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/3",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                93,
                94,
                95
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/3",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/3",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/3",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                93
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/3",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "93": {
        "mapping": {
          "id": 93,
          "name": "eth101/3/2",
          "controllingPort": 92,
          "pins": [
            {
              "a": {
                "chip": "FC22",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth101/3",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/3",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/3",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "94": {
        "mapping": {
          "id": 94,
          "name": "eth101/3/3",
          "controllingPort": 92,
          "pins": [
            {
              "a": {
                "chip": "FC22",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth101/3",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/3",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/3",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                95
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/3",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "95": {
        "mapping": {
          "id": 95,
          "name": "eth101/3/4",
          "controllingPort": 92,
          "pins": [
            {
              "a": {
                "chip": "FC22",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth101/3",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/3",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/3",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "96": {
        "mapping": {
          "id": 96,
          "name": "eth101/4/1",
          "controllingPort": 96,
          "pins": [
            {
              "a": {
                "chip": "FC23",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth101/4",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/4",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                97,
                98,
                99
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/4",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/4",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                97,
                98,
                99
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/4",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/4",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/4",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                97
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/4",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "97": {
        "mapping": {
          "id": 97,
          "name": "eth101/4/2",
          "controllingPort": 96,
          "pins": [
            {
              "a": {
                "chip": "FC23",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth101/4",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/4",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/4",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "98": {
        "mapping": {
          "id": 98,
          "name": "eth101/4/3",
          "controllingPort": 96,
          "pins": [
            {
              "a": {
                "chip": "FC23",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth101/4",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/4",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/4",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                99
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/4",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "99": {
        "mapping": {
          "id": 99,
          "name": "eth101/4/4",
          "controllingPort": 96,
          "pins": [
            {
              "a": {
                "chip": "FC23",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth101/4",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/4",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/4",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "102": {
        "mapping": {
          "id": 102,
          "name": "eth101/5/1",
          "controllingPort": 102,
          "pins": [
            {
              "a": {
                "chip": "FC24",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth101/5",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/5",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                103,
                104,
                105
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/5",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/5",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                103,
                104,
                105
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/5",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/5",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/5",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                103
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/5",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "103": {
        "mapping": {
          "id": 103,
          "name": "eth101/5/2",
          "controllingPort": 102,
          "pins": [
            {
              "a": {
                "chip": "FC24",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth101/5",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/5",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/5",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "104": {
        "mapping": {
          "id": 104,
          "name": "eth101/5/3",
          "controllingPort": 102,
          "pins": [
            {
              "a": {
                "chip": "FC24",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth101/5",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/5",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/5",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                105
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/5",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "105": {
        "mapping": {
          "id": 105,
          "name": "eth101/5/4",
          "controllingPort": 102,
          "pins": [
            {
              "a": {
                "chip": "FC24",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth101/5",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/5",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/5",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "106": {
        "mapping": {
          "id": 106,
          "name": "eth101/6/1",
          "controllingPort": 106,
          "pins": [
            {
              "a": {
                "chip": "FC25",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth101/6",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/6",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                107,
                108,
                109
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/6",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/6",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                107,
                108,
                109
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/6",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/6",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/6",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                107
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/6",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "107": {
        "mapping": {
          "id": 107,
          "name": "eth101/6/2",
          "controllingPort": 106,
          "pins": [
            {
              "a": {
                "chip": "FC25",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth101/6",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/6",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/6",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "108": {
        "mapping": {
          "id": 108,
          "name": "eth101/6/3",
          "controllingPort": 106,
          "pins": [
            {
              "a": {
                "chip": "FC25",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth101/6",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/6",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/6",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                109
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/6",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "109": {
        "mapping": {
          "id": 109,
          "name": "eth101/6/4",
          "controllingPort": 106,
          "pins": [
            {
              "a": {
                "chip": "FC25",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth101/6",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/6",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/6",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "110": {
        "mapping": {
          "id": 110,
          "name": "eth101/7/1",
          "controllingPort": 110,
          "pins": [
            {
              "a": {
                "chip": "FC26",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth101/7",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/7",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                111,
                112,
                113
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/7",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/7",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                111,
                112,
                113
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/7",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/7",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/7",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                111
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/7",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "111": {
        "mapping": {
          "id": 111,
          "name": "eth101/7/2",
          "controllingPort": 110,
          "pins": [
            {
              "a": {
                "chip": "FC26",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth101/7",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/7",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/7",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "112": {
        "mapping": {
          "id": 112,
          "name": "eth101/7/3",
          "controllingPort": 110,
          "pins": [
            {
              "a": {
                "chip": "FC26",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth101/7",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/7",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/7",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                113
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/7",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "113": {
        "mapping": {
          "id": 113,
          "name": "eth101/7/4",
          "controllingPort": 110,
          "pins": [
            {
              "a": {
                "chip": "FC26",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth101/7",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/7",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/7",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "114": {
        "mapping": {
          "id": 114,
          "name": "eth101/8/1",
          "controllingPort": 114,
          "pins": [
            {
              "a": {
                "chip": "FC27",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth101/8",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/8",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                115,
                116,
                117
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/8",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/8",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                115,
                116,
                117
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/8",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/8",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/8",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                115
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/8",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "115": {
        "mapping": {
          "id": 115,
          "name": "eth101/8/2",
          "controllingPort": 114,
          "pins": [
            {
              "a": {
                "chip": "FC27",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth101/8",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/8",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/8",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "116": {
        "mapping": {
          "id": 116,
          "name": "eth101/8/3",
          "controllingPort": 114,
          "pins": [
            {
              "a": {
                "chip": "FC27",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth101/8",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/8",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/8",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                117
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/8",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "117": {
        "mapping": {
          "id": 117,
          "name": "eth101/8/4",
          "controllingPort": 114,
          "pins": [
            {
              "a": {
                "chip": "FC27",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth101/8",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/8",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/8",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "118": {
        "mapping": {
          "id": 118,
          "name": "eth101/9/1",
          "controllingPort": 118,
          "pins": [
            {
              "a": {
                "chip": "FC28",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth101/9",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/9",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                119,
                120,
                121
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/9",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/9",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                119,
                120,
                121
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/9",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/9",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/9",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                119
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/9",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "119": {
        "mapping": {
          "id": 119,
          "name": "eth101/9/2",
          "controllingPort": 118,
          "pins": [
            {
              "a": {
                "chip": "FC28",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth101/9",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/9",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/9",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "120": {
        "mapping": {
          "id": 120,
          "name": "eth101/9/3",
          "controllingPort": 118,
          "pins": [
            {
              "a": {
                "chip": "FC28",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth101/9",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/9",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/9",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                121
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/9",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "121": {
        "mapping": {
          "id": 121,
          "name": "eth101/9/4",
          "controllingPort": 118,
          "pins": [
            {
              "a": {
                "chip": "FC28",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth101/9",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/9",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/9",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "122": {
        "mapping": {
          "id": 122,
          "name": "eth101/10/1",
          "controllingPort": 122,
          "pins": [
            {
              "a": {
                "chip": "FC29",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth101/10",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/10",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                123,
                124,
                125
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/10",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/10",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                123,
                124,
                125
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/10",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/10",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/10",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                123
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/10",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "123": {
        "mapping": {
          "id": 123,
          "name": "eth101/10/2",
          "controllingPort": 122,
          "pins": [
            {
              "a": {
                "chip": "FC29",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth101/10",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/10",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/10",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "124": {
        "mapping": {
          "id": 124,
          "name": "eth101/10/3",
          "controllingPort": 122,
          "pins": [
            {
              "a": {
                "chip": "FC29",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth101/10",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/10",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/10",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                125
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/10",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "125": {
        "mapping": {
          "id": 125,
          "name": "eth101/10/4",
          "controllingPort": 122,
          "pins": [
            {
              "a": {
                "chip": "FC29",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth101/10",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/10",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/10",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "126": {
        "mapping": {
          "id": 126,
          "name": "eth101/11/1",
          "controllingPort": 126,
          "pins": [
            {
              "a": {
                "chip": "FC30",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth101/11",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/11",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                127,
                128,
                129
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/11",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/11",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                127,
                128,
                129
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/11",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/11",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/11",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                127
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/11",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "127": {
        "mapping": {
          "id": 127,
          "name": "eth101/11/2",
          "controllingPort": 126,
          "pins": [
            {
              "a": {
                "chip": "FC30",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth101/11",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/11",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/11",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "128": {
        "mapping": {
          "id": 128,
          "name": "eth101/11/3",
          "controllingPort": 126,
          "pins": [
            {
              "a": {
                "chip": "FC30",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth101/11",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/11",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/11",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                129
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/11",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "129": {
        "mapping": {
          "id": 129,
          "name": "eth101/11/4",
          "controllingPort": 126,
          "pins": [
            {
              "a": {
                "chip": "FC30",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth101/11",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/11",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/11",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "130": {
        "mapping": {
          "id": 130,
          "name": "eth101/12/1",
          "controllingPort": 130,
          "pins": [
            {
              "a": {
                "chip": "FC31",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth101/12",
                  "lane": 0
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/12",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "18": {
              "subsumedPorts": [
                131,
                132,
                133
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/12",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/12",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "28": {
              "subsumedPorts": [
                131,
                132,
                133
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/12",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/12",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/12",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                131
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/12",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "131": {
        "mapping": {
          "id": 131,
          "name": "eth101/12/2",
          "controllingPort": 130,
          "pins": [
            {
              "a": {
                "chip": "FC31",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth101/12",
                  "lane": 1
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/12",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/12",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "132": {
        "mapping": {
          "id": 132,
          "name": "eth101/12/3",
          "controllingPort": 130,
          "pins": [
            {
              "a": {
                "chip": "FC31",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth101/12",
                  "lane": 2
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/12",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/12",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "31": {
              "subsumedPorts": [
                133
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth101/12",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "133": {
        "mapping": {
          "id": 133,
          "name": "eth101/12/4",
          "controllingPort": 130,
          "pins": [
            {
              "a": {
                "chip": "FC31",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth101/12",
                  "lane": 3
                }
              }
            }
          ]
        },
        "supportedProfiles": {
          "12": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/12",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "30": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth101/12",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    }
  },
  "supportedProfiles": {
    "12": {
        "speed": 10000,
        "iphy": {
          "numLanes": 1,
          "modulation": 1,
          "fec": 1,
          "medium": 2,
          "interfaceMode": 41,
          "interfaceType": 41
        }
    },
    "14": {
        "speed": 25000,
        "iphy": {
          "numLanes": 1,
          "modulation": 1,
          "fec": 1,
          "medium": 1,
          "interfaceMode": 10,
          "interfaceType": 10
        }
    },
    "18": {
        "speed": 40000,
        "iphy": {
          "numLanes": 4,
          "modulation": 1,
          "fec": 1,
          "medium": 2,
          "interfaceMode": 40,
          "interfaceType": 40
        }
    },
    "19": {
        "speed": 50000,
        "iphy": {
          "numLanes": 2,
          "modulation": 1,
          "fec": 1,
          "medium": 1,
          "interfaceMode": 11,
          "interfaceType": 11
        }
    },
    "28": {
        "speed": 100000,
        "iphy": {
          "numLanes": 4,
          "modulation": 1,
          "fec": 91,
          "medium": 2,
          "interfaceMode": 30,
          "interfaceType": 30
        }
    },
    "30": {
        "speed": 25000,
        "iphy": {
          "numLanes": 1,
          "modulation": 1,
          "fec": 1,
          "medium": 2,
          "interfaceMode": 30,
          "interfaceType": 30
        }
    },
    "31": {
        "speed": 50000,
        "iphy": {
          "numLanes": 2,
          "modulation": 1,
          "fec": 1,
          "medium": 2,
          "interfaceMode": 30,
          "interfaceType": 30
        }
    },
    "32": {
        "speed": 100000,
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
  "chips": [
    {
      "name": "FC20",
      "type": 1,
      "physicalID": 20
    },
    {
      "name": "FC21",
      "type": 1,
      "physicalID": 21
    },
    {
      "name": "FC22",
      "type": 1,
      "physicalID": 22
    },
    {
      "name": "FC23",
      "type": 1,
      "physicalID": 23
    },
    {
      "name": "FC24",
      "type": 1,
      "physicalID": 24
    },
    {
      "name": "FC25",
      "type": 1,
      "physicalID": 25
    },
    {
      "name": "FC26",
      "type": 1,
      "physicalID": 26
    },
    {
      "name": "FC27",
      "type": 1,
      "physicalID": 27
    },
    {
      "name": "FC28",
      "type": 1,
      "physicalID": 28
    },
    {
      "name": "FC29",
      "type": 1,
      "physicalID": 29
    },
    {
      "name": "FC30",
      "type": 1,
      "physicalID": 30
    },
    {
      "name": "FC31",
      "type": 1,
      "physicalID": 31
    },
    {
      "name": "FC0",
      "type": 1,
      "physicalID": 0
    },
    {
      "name": "FC1",
      "type": 1,
      "physicalID": 1
    },
    {
      "name": "FC2",
      "type": 1,
      "physicalID": 2
    },
    {
      "name": "FC3",
      "type": 1,
      "physicalID": 3
    },
    {
      "name": "FC16",
      "type": 1,
      "physicalID": 16
    },
    {
      "name": "FC17",
      "type": 1,
      "physicalID": 17
    },
    {
      "name": "FC18",
      "type": 1,
      "physicalID": 18
    },
    {
      "name": "FC19",
      "type": 1,
      "physicalID": 19
    },
    {
      "name": "FC12",
      "type": 1,
      "physicalID": 12
    },
    {
      "name": "FC13",
      "type": 1,
      "physicalID": 13
    },
    {
      "name": "FC14",
      "type": 1,
      "physicalID": 14
    },
    {
      "name": "FC15",
      "type": 1,
      "physicalID": 15
    },
    {
      "name": "FC8",
      "type": 1,
      "physicalID": 8
    },
    {
      "name": "FC9",
      "type": 1,
      "physicalID": 9
    },
    {
      "name": "FC10",
      "type": 1,
      "physicalID": 10
    },
    {
      "name": "FC11",
      "type": 1,
      "physicalID": 11
    },
    {
      "name": "FC4",
      "type": 1,
      "physicalID": 4
    },
    {
      "name": "FC5",
      "type": 1,
      "physicalID": 5
    },
    {
      "name": "FC6",
      "type": 1,
      "physicalID": 6
    },
    {
      "name": "FC7",
      "type": 1,
      "physicalID": 7
    },
    {
      "name": "eth101/1",
      "type": 3,
      "physicalID": 0
    },
    {
      "name": "eth101/2",
      "type": 3,
      "physicalID": 1
    },
    {
      "name": "eth101/3",
      "type": 3,
      "physicalID": 2
    },
    {
      "name": "eth101/4",
      "type": 3,
      "physicalID": 3
    },
    {
      "name": "eth101/5",
      "type": 3,
      "physicalID": 4
    },
    {
      "name": "eth101/6",
      "type": 3,
      "physicalID": 5
    },
    {
      "name": "eth101/7",
      "type": 3,
      "physicalID": 6
    },
    {
      "name": "eth101/8",
      "type": 3,
      "physicalID": 7
    },
    {
      "name": "eth101/9",
      "type": 3,
      "physicalID": 8
    },
    {
      "name": "eth101/10",
      "type": 3,
      "physicalID": 9
    },
    {
      "name": "eth101/11",
      "type": 3,
      "physicalID": 10
    },
    {
      "name": "eth101/12",
      "type": 3,
      "physicalID": 11
    },
    {
      "name": "eth101/13",
      "type": 3,
      "physicalID": 12
    },
    {
      "name": "eth101/14",
      "type": 3,
      "physicalID": 13
    },
    {
      "name": "eth101/15",
      "type": 3,
      "physicalID": 14
    },
    {
      "name": "eth101/16",
      "type": 3,
      "physicalID": 15
    }
  ]
}
)";

constexpr auto kLineCardNameRegex = "lc(\\d+)";
constexpr auto kDefaultLCName = "lc101";
constexpr auto kLCName = "lc_name";

std::string updatePlatformMappingStr(const std::string& lcName) {
  int cardID = 0;
  re2::RE2 cardIDRe(kLineCardNameRegex);
  if (!re2::RE2::FullMatch(lcName, cardIDRe, &cardID)) {
    throw facebook::fboss::FbossError("Invalid line card name:", lcName);
  }
  std::string tmpStr = kJsonPlatformMappingStr;
  boost::replace_all(tmpStr, "fab101", folly::to<std::string>("fab", cardID));
  boost::replace_all(tmpStr, "eth101", folly::to<std::string>("eth", cardID));
  return tmpStr;
}
} // namespace

namespace facebook {
namespace fboss {
GalaxyLCPlatformMapping::GalaxyLCPlatformMapping(
    const std::string& linecardName)
    : PlatformMapping(updatePlatformMappingStr(linecardName)) {}

std::string GalaxyLCPlatformMapping::getLinecardName() {
  std::string netwhoamiStr;
  if (!folly::readFile(FLAGS_netwhoami.data(), netwhoamiStr)) {
    return kDefaultLCName;
  }

  auto netwhoamiDynamic = folly::parseJson(netwhoamiStr);
  if (netwhoamiDynamic.find(kLCName) == netwhoamiDynamic.items().end()) {
    return kDefaultLCName;
  }
  return netwhoamiDynamic[kLCName].asString();
}
} // namespace fboss
} // namespace facebook
