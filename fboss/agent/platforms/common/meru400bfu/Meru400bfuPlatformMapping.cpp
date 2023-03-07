/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/meru400bfu/Meru400bfuPlatformMapping.h"

namespace facebook::fboss {
namespace {
constexpr auto kJsonPlatformMappingStr = R"(
{
  "ports": {
    "0": {
        "mapping": {
          "id": 0,
          "name": "fab1/12/1",
          "controllingPort": 0,
          "pins": [
            {
              "a": {
                "chip": "BC0",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/12",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "1": {
        "mapping": {
          "id": 1,
          "name": "fab1/12/2",
          "controllingPort": 1,
          "pins": [
            {
              "a": {
                "chip": "BC0",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/12",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC0",
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
          "name": "fab1/12/3",
          "controllingPort": 2,
          "pins": [
            {
              "a": {
                "chip": "BC0",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/12",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 2
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
          "name": "fab1/12/4",
          "controllingPort": 3,
          "pins": [
            {
              "a": {
                "chip": "BC0",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/12",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC0",
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
          "name": "fab1/11/1",
          "controllingPort": 4,
          "pins": [
            {
              "a": {
                "chip": "BC1",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/11",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 0
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
          "name": "fab1/11/2",
          "controllingPort": 5,
          "pins": [
            {
              "a": {
                "chip": "BC1",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/11",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC1",
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
          "name": "fab1/11/3",
          "controllingPort": 6,
          "pins": [
            {
              "a": {
                "chip": "BC1",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/11",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 2
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
          "name": "fab1/11/4",
          "controllingPort": 7,
          "pins": [
            {
              "a": {
                "chip": "BC1",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/11",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC1",
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
          "name": "fab1/9/1",
          "controllingPort": 8,
          "pins": [
            {
              "a": {
                "chip": "BC2",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/9",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 0
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
          "name": "fab1/9/2",
          "controllingPort": 9,
          "pins": [
            {
              "a": {
                "chip": "BC2",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/9",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC2",
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
          "name": "fab1/9/3",
          "controllingPort": 10,
          "pins": [
            {
              "a": {
                "chip": "BC2",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/9",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 2
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
          "name": "fab1/9/4",
          "controllingPort": 11,
          "pins": [
            {
              "a": {
                "chip": "BC2",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/9",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC2",
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
          "name": "fab1/10/1",
          "controllingPort": 12,
          "pins": [
            {
              "a": {
                "chip": "BC3",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/10",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 0
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
          "name": "fab1/10/2",
          "controllingPort": 13,
          "pins": [
            {
              "a": {
                "chip": "BC3",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/10",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC3",
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
          "name": "fab1/10/3",
          "controllingPort": 14,
          "pins": [
            {
              "a": {
                "chip": "BC3",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/10",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 2
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
          "name": "fab1/10/4",
          "controllingPort": 15,
          "pins": [
            {
              "a": {
                "chip": "BC3",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/10",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC3",
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
          "name": "fab1/7/1",
          "controllingPort": 16,
          "pins": [
            {
              "a": {
                "chip": "BC4",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/7",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 0
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
          "name": "fab1/7/2",
          "controllingPort": 17,
          "pins": [
            {
              "a": {
                "chip": "BC4",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/7",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC4",
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
          "name": "fab1/7/3",
          "controllingPort": 18,
          "pins": [
            {
              "a": {
                "chip": "BC4",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/7",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 2
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
          "name": "fab1/7/4",
          "controllingPort": 19,
          "pins": [
            {
              "a": {
                "chip": "BC4",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/7",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC4",
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
          "name": "fab1/8/1",
          "controllingPort": 20,
          "pins": [
            {
              "a": {
                "chip": "BC5",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/8",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 0
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
          "name": "fab1/8/2",
          "controllingPort": 21,
          "pins": [
            {
              "a": {
                "chip": "BC5",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/8",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC5",
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
          "name": "fab1/8/3",
          "controllingPort": 22,
          "pins": [
            {
              "a": {
                "chip": "BC5",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/8",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 2
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
          "name": "fab1/8/4",
          "controllingPort": 23,
          "pins": [
            {
              "a": {
                "chip": "BC5",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/8",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC5",
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
          "name": "fab1/6/1",
          "controllingPort": 24,
          "pins": [
            {
              "a": {
                "chip": "BC6",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/6",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 0
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
          "name": "fab1/6/2",
          "controllingPort": 25,
          "pins": [
            {
              "a": {
                "chip": "BC6",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/6",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC6",
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
          "name": "fab1/6/3",
          "controllingPort": 26,
          "pins": [
            {
              "a": {
                "chip": "BC6",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/6",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 2
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
          "name": "fab1/6/4",
          "controllingPort": 27,
          "pins": [
            {
              "a": {
                "chip": "BC6",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/6",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC6",
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
          "name": "fab1/5/1",
          "controllingPort": 28,
          "pins": [
            {
              "a": {
                "chip": "BC7",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/5",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 0
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
          "name": "fab1/5/2",
          "controllingPort": 29,
          "pins": [
            {
              "a": {
                "chip": "BC7",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/5",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC7",
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
          "name": "fab1/5/3",
          "controllingPort": 30,
          "pins": [
            {
              "a": {
                "chip": "BC7",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/5",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 2
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
          "name": "fab1/5/4",
          "controllingPort": 31,
          "pins": [
            {
              "a": {
                "chip": "BC7",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/5",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC7",
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
          "name": "fab1/36/5",
          "controllingPort": 32,
          "pins": [
            {
              "a": {
                "chip": "BC8",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/36",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 0
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
          "name": "fab1/36/6",
          "controllingPort": 33,
          "pins": [
            {
              "a": {
                "chip": "BC8",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/36",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC8",
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
          "name": "fab1/36/7",
          "controllingPort": 34,
          "pins": [
            {
              "a": {
                "chip": "BC8",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/36",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 2
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
          "name": "fab1/36/8",
          "controllingPort": 35,
          "pins": [
            {
              "a": {
                "chip": "BC8",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/36",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC8",
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
          "name": "fab1/4/1",
          "controllingPort": 36,
          "pins": [
            {
              "a": {
                "chip": "BC9",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/4",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 0
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
          "name": "fab1/4/2",
          "controllingPort": 37,
          "pins": [
            {
              "a": {
                "chip": "BC9",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/4",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC9",
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
          "name": "fab1/4/3",
          "controllingPort": 38,
          "pins": [
            {
              "a": {
                "chip": "BC9",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/4",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 2
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
          "name": "fab1/4/4",
          "controllingPort": 39,
          "pins": [
            {
              "a": {
                "chip": "BC9",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/4",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC9",
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
          "name": "fab1/35/5",
          "controllingPort": 40,
          "pins": [
            {
              "a": {
                "chip": "BC10",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/35",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 0
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
          "name": "fab1/35/6",
          "controllingPort": 41,
          "pins": [
            {
              "a": {
                "chip": "BC10",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/35",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC10",
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
          "name": "fab1/35/7",
          "controllingPort": 42,
          "pins": [
            {
              "a": {
                "chip": "BC10",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/35",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 2
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
          "name": "fab1/35/8",
          "controllingPort": 43,
          "pins": [
            {
              "a": {
                "chip": "BC10",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/35",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC10",
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
          "name": "fab1/34/5",
          "controllingPort": 44,
          "pins": [
            {
              "a": {
                "chip": "BC11",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/34",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 0
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
          "name": "fab1/34/6",
          "controllingPort": 45,
          "pins": [
            {
              "a": {
                "chip": "BC11",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/34",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC11",
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
          "name": "fab1/34/7",
          "controllingPort": 46,
          "pins": [
            {
              "a": {
                "chip": "BC11",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/34",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 2
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
          "name": "fab1/34/8",
          "controllingPort": 47,
          "pins": [
            {
              "a": {
                "chip": "BC11",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/34",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC11",
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
          "name": "fab1/25/5",
          "controllingPort": 48,
          "pins": [
            {
              "a": {
                "chip": "BC12",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/25",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 0
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
          "name": "fab1/25/6",
          "controllingPort": 49,
          "pins": [
            {
              "a": {
                "chip": "BC12",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/25",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC12",
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
          "name": "fab1/25/7",
          "controllingPort": 50,
          "pins": [
            {
              "a": {
                "chip": "BC12",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/25",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 2
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
          "name": "fab1/25/8",
          "controllingPort": 51,
          "pins": [
            {
              "a": {
                "chip": "BC12",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/25",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC12",
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
          "name": "fab1/26/5",
          "controllingPort": 52,
          "pins": [
            {
              "a": {
                "chip": "BC13",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/26",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 0
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
          "name": "fab1/26/6",
          "controllingPort": 53,
          "pins": [
            {
              "a": {
                "chip": "BC13",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/26",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC13",
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
          "name": "fab1/26/7",
          "controllingPort": 54,
          "pins": [
            {
              "a": {
                "chip": "BC13",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/26",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 2
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
          "name": "fab1/26/8",
          "controllingPort": 55,
          "pins": [
            {
              "a": {
                "chip": "BC13",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/26",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC13",
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
          "name": "fab1/28/5",
          "controllingPort": 56,
          "pins": [
            {
              "a": {
                "chip": "BC14",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/28",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 0
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
          "name": "fab1/28/6",
          "controllingPort": 57,
          "pins": [
            {
              "a": {
                "chip": "BC14",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/28",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC14",
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
          "name": "fab1/28/7",
          "controllingPort": 58,
          "pins": [
            {
              "a": {
                "chip": "BC14",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/28",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 2
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
          "name": "fab1/28/8",
          "controllingPort": 59,
          "pins": [
            {
              "a": {
                "chip": "BC14",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/28",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC14",
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
          "name": "fab1/27/5",
          "controllingPort": 60,
          "pins": [
            {
              "a": {
                "chip": "BC15",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/27",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 0
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
          "name": "fab1/27/6",
          "controllingPort": 61,
          "pins": [
            {
              "a": {
                "chip": "BC15",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/27",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC15",
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
          "name": "fab1/27/7",
          "controllingPort": 62,
          "pins": [
            {
              "a": {
                "chip": "BC15",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/27",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 2
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
          "name": "fab1/27/8",
          "controllingPort": 63,
          "pins": [
            {
              "a": {
                "chip": "BC15",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/27",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC15",
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
          "name": "fab1/30/5",
          "controllingPort": 64,
          "pins": [
            {
              "a": {
                "chip": "BC16",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/30",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 0
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
          "name": "fab1/30/6",
          "controllingPort": 65,
          "pins": [
            {
              "a": {
                "chip": "BC16",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/30",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "66": {
        "mapping": {
          "id": 66,
          "name": "fab1/30/7",
          "controllingPort": 66,
          "pins": [
            {
              "a": {
                "chip": "BC16",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/30",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "67": {
        "mapping": {
          "id": 67,
          "name": "fab1/30/8",
          "controllingPort": 67,
          "pins": [
            {
              "a": {
                "chip": "BC16",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/30",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC16",
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
          "name": "fab1/29/5",
          "controllingPort": 68,
          "pins": [
            {
              "a": {
                "chip": "BC17",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/29",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 0
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
          "name": "fab1/29/6",
          "controllingPort": 69,
          "pins": [
            {
              "a": {
                "chip": "BC17",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/29",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC17",
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
          "name": "fab1/29/7",
          "controllingPort": 70,
          "pins": [
            {
              "a": {
                "chip": "BC17",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/29",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 2
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
          "name": "fab1/29/8",
          "controllingPort": 71,
          "pins": [
            {
              "a": {
                "chip": "BC17",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/29",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC17",
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
          "name": "fab1/1/1",
          "controllingPort": 72,
          "pins": [
            {
              "a": {
                "chip": "BC18",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/1",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 0
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
          "name": "fab1/1/2",
          "controllingPort": 73,
          "pins": [
            {
              "a": {
                "chip": "BC18",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/1",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC18",
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
          "name": "fab1/1/3",
          "controllingPort": 74,
          "pins": [
            {
              "a": {
                "chip": "BC18",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/1",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 2
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
          "name": "fab1/1/4",
          "controllingPort": 75,
          "pins": [
            {
              "a": {
                "chip": "BC18",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/1",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC18",
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
          "name": "fab1/2/1",
          "controllingPort": 76,
          "pins": [
            {
              "a": {
                "chip": "BC19",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/2",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 0
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
          "name": "fab1/2/2",
          "controllingPort": 77,
          "pins": [
            {
              "a": {
                "chip": "BC19",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/2",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC19",
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
          "name": "fab1/2/3",
          "controllingPort": 78,
          "pins": [
            {
              "a": {
                "chip": "BC19",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/2",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 2
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
          "name": "fab1/2/4",
          "controllingPort": 79,
          "pins": [
            {
              "a": {
                "chip": "BC19",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/2",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC19",
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
          "name": "fab1/31/5",
          "controllingPort": 80,
          "pins": [
            {
              "a": {
                "chip": "BC20",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/31",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 0
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
          "name": "fab1/31/6",
          "controllingPort": 81,
          "pins": [
            {
              "a": {
                "chip": "BC20",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/31",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC20",
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
          "name": "fab1/31/7",
          "controllingPort": 82,
          "pins": [
            {
              "a": {
                "chip": "BC20",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/31",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 2
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
          "name": "fab1/31/8",
          "controllingPort": 83,
          "pins": [
            {
              "a": {
                "chip": "BC20",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/31",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC20",
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
          "name": "fab1/3/1",
          "controllingPort": 84,
          "pins": [
            {
              "a": {
                "chip": "BC21",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/3",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 0
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
          "name": "fab1/3/2",
          "controllingPort": 85,
          "pins": [
            {
              "a": {
                "chip": "BC21",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/3",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC21",
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
          "name": "fab1/3/3",
          "controllingPort": 86,
          "pins": [
            {
              "a": {
                "chip": "BC21",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/3",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 2
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
          "name": "fab1/3/4",
          "controllingPort": 87,
          "pins": [
            {
              "a": {
                "chip": "BC21",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/3",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC21",
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
          "name": "fab1/32/5",
          "controllingPort": 88,
          "pins": [
            {
              "a": {
                "chip": "BC22",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/32",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 0
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
          "name": "fab1/32/6",
          "controllingPort": 89,
          "pins": [
            {
              "a": {
                "chip": "BC22",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/32",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC22",
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
          "name": "fab1/32/7",
          "controllingPort": 90,
          "pins": [
            {
              "a": {
                "chip": "BC22",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/32",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 2
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
          "name": "fab1/32/8",
          "controllingPort": 91,
          "pins": [
            {
              "a": {
                "chip": "BC22",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/32",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC22",
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
          "name": "fab1/33/5",
          "controllingPort": 92,
          "pins": [
            {
              "a": {
                "chip": "BC23",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/33",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 0
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
          "name": "fab1/33/6",
          "controllingPort": 93,
          "pins": [
            {
              "a": {
                "chip": "BC23",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/33",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC23",
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
          "name": "fab1/33/7",
          "controllingPort": 94,
          "pins": [
            {
              "a": {
                "chip": "BC23",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/33",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 2
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
          "name": "fab1/33/8",
          "controllingPort": 95,
          "pins": [
            {
              "a": {
                "chip": "BC23",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/33",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC23",
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
          "name": "fab1/48/5",
          "controllingPort": 96,
          "pins": [
            {
              "a": {
                "chip": "BC24",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/48",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 0
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
          "name": "fab1/48/6",
          "controllingPort": 97,
          "pins": [
            {
              "a": {
                "chip": "BC24",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/48",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC24",
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
          "name": "fab1/48/7",
          "controllingPort": 98,
          "pins": [
            {
              "a": {
                "chip": "BC24",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/48",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 2
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
          "name": "fab1/48/8",
          "controllingPort": 99,
          "pins": [
            {
              "a": {
                "chip": "BC24",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/48",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "100": {
        "mapping": {
          "id": 100,
          "name": "fab1/47/5",
          "controllingPort": 100,
          "pins": [
            {
              "a": {
                "chip": "BC25",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/47",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "101": {
        "mapping": {
          "id": 101,
          "name": "fab1/47/6",
          "controllingPort": 101,
          "pins": [
            {
              "a": {
                "chip": "BC25",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/47",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 1
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
          "name": "fab1/47/7",
          "controllingPort": 102,
          "pins": [
            {
              "a": {
                "chip": "BC25",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/47",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 2
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
          "name": "fab1/47/8",
          "controllingPort": 103,
          "pins": [
            {
              "a": {
                "chip": "BC25",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/47",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 3
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
          "name": "fab1/45/5",
          "controllingPort": 104,
          "pins": [
            {
              "a": {
                "chip": "BC26",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/45",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 0
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
          "name": "fab1/45/6",
          "controllingPort": 105,
          "pins": [
            {
              "a": {
                "chip": "BC26",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/45",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 1
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
          "name": "fab1/45/7",
          "controllingPort": 106,
          "pins": [
            {
              "a": {
                "chip": "BC26",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/45",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 2
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
          "name": "fab1/45/8",
          "controllingPort": 107,
          "pins": [
            {
              "a": {
                "chip": "BC26",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/45",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 3
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
          "name": "fab1/46/5",
          "controllingPort": 108,
          "pins": [
            {
              "a": {
                "chip": "BC27",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/46",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 0
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
          "name": "fab1/46/6",
          "controllingPort": 109,
          "pins": [
            {
              "a": {
                "chip": "BC27",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/46",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 1
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
          "name": "fab1/46/7",
          "controllingPort": 110,
          "pins": [
            {
              "a": {
                "chip": "BC27",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/46",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 2
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
          "name": "fab1/46/8",
          "controllingPort": 111,
          "pins": [
            {
              "a": {
                "chip": "BC27",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/46",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 3
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
          "name": "fab1/43/5",
          "controllingPort": 112,
          "pins": [
            {
              "a": {
                "chip": "BC28",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/43",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 0
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
          "name": "fab1/43/6",
          "controllingPort": 113,
          "pins": [
            {
              "a": {
                "chip": "BC28",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/43",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 1
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
          "name": "fab1/43/7",
          "controllingPort": 114,
          "pins": [
            {
              "a": {
                "chip": "BC28",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/43",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 2
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
          "name": "fab1/43/8",
          "controllingPort": 115,
          "pins": [
            {
              "a": {
                "chip": "BC28",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/43",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 3
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
          "name": "fab1/44/5",
          "controllingPort": 116,
          "pins": [
            {
              "a": {
                "chip": "BC29",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/44",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 0
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
          "name": "fab1/44/6",
          "controllingPort": 117,
          "pins": [
            {
              "a": {
                "chip": "BC29",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/44",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 1
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
          "name": "fab1/44/7",
          "controllingPort": 118,
          "pins": [
            {
              "a": {
                "chip": "BC29",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/44",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 2
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
          "name": "fab1/44/8",
          "controllingPort": 119,
          "pins": [
            {
              "a": {
                "chip": "BC29",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/44",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 3
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
          "name": "fab1/23/1",
          "controllingPort": 120,
          "pins": [
            {
              "a": {
                "chip": "BC30",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/23",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 0
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
          "name": "fab1/23/2",
          "controllingPort": 121,
          "pins": [
            {
              "a": {
                "chip": "BC30",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/23",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 1
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
          "name": "fab1/23/3",
          "controllingPort": 122,
          "pins": [
            {
              "a": {
                "chip": "BC30",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/23",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 2
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
          "name": "fab1/23/4",
          "controllingPort": 123,
          "pins": [
            {
              "a": {
                "chip": "BC30",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/23",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 3
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
          "name": "fab1/24/1",
          "controllingPort": 124,
          "pins": [
            {
              "a": {
                "chip": "BC31",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/24",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 0
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
          "name": "fab1/24/2",
          "controllingPort": 125,
          "pins": [
            {
              "a": {
                "chip": "BC31",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/24",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 1
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
          "name": "fab1/24/3",
          "controllingPort": 126,
          "pins": [
            {
              "a": {
                "chip": "BC31",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/24",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 2
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
          "name": "fab1/24/4",
          "controllingPort": 127,
          "pins": [
            {
              "a": {
                "chip": "BC31",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/24",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 3
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
          "name": "fab1/42/5",
          "controllingPort": 128,
          "pins": [
            {
              "a": {
                "chip": "BC32",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/42",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 0
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
          "name": "fab1/42/6",
          "controllingPort": 129,
          "pins": [
            {
              "a": {
                "chip": "BC32",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/42",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 1
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
          "name": "fab1/42/7",
          "controllingPort": 130,
          "pins": [
            {
              "a": {
                "chip": "BC32",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/42",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 2
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
          "name": "fab1/42/8",
          "controllingPort": 131,
          "pins": [
            {
              "a": {
                "chip": "BC32",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/42",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 3
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
          "name": "fab1/21/1",
          "controllingPort": 132,
          "pins": [
            {
              "a": {
                "chip": "BC33",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/21",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 0
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
          "name": "fab1/21/2",
          "controllingPort": 133,
          "pins": [
            {
              "a": {
                "chip": "BC33",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/21",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "134": {
        "mapping": {
          "id": 134,
          "name": "fab1/21/3",
          "controllingPort": 134,
          "pins": [
            {
              "a": {
                "chip": "BC33",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/21",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "135": {
        "mapping": {
          "id": 135,
          "name": "fab1/21/4",
          "controllingPort": 135,
          "pins": [
            {
              "a": {
                "chip": "BC33",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/21",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "136": {
        "mapping": {
          "id": 136,
          "name": "fab1/41/5",
          "controllingPort": 136,
          "pins": [
            {
              "a": {
                "chip": "BC34",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/41",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "137": {
        "mapping": {
          "id": 137,
          "name": "fab1/41/6",
          "controllingPort": 137,
          "pins": [
            {
              "a": {
                "chip": "BC34",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/41",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "138": {
        "mapping": {
          "id": 138,
          "name": "fab1/41/7",
          "controllingPort": 138,
          "pins": [
            {
              "a": {
                "chip": "BC34",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/41",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "139": {
        "mapping": {
          "id": 139,
          "name": "fab1/41/8",
          "controllingPort": 139,
          "pins": [
            {
              "a": {
                "chip": "BC34",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/41",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "140": {
        "mapping": {
          "id": 140,
          "name": "fab1/40/5",
          "controllingPort": 140,
          "pins": [
            {
              "a": {
                "chip": "BC35",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/40",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "141": {
        "mapping": {
          "id": 141,
          "name": "fab1/40/6",
          "controllingPort": 141,
          "pins": [
            {
              "a": {
                "chip": "BC35",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/40",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "142": {
        "mapping": {
          "id": 142,
          "name": "fab1/40/7",
          "controllingPort": 142,
          "pins": [
            {
              "a": {
                "chip": "BC35",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/40",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "143": {
        "mapping": {
          "id": 143,
          "name": "fab1/40/8",
          "controllingPort": 143,
          "pins": [
            {
              "a": {
                "chip": "BC35",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/40",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "144": {
        "mapping": {
          "id": 144,
          "name": "fab1/14/1",
          "controllingPort": 144,
          "pins": [
            {
              "a": {
                "chip": "BC36",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/14",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "145": {
        "mapping": {
          "id": 145,
          "name": "fab1/14/2",
          "controllingPort": 145,
          "pins": [
            {
              "a": {
                "chip": "BC36",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/14",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "146": {
        "mapping": {
          "id": 146,
          "name": "fab1/14/3",
          "controllingPort": 146,
          "pins": [
            {
              "a": {
                "chip": "BC36",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/14",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "147": {
        "mapping": {
          "id": 147,
          "name": "fab1/14/4",
          "controllingPort": 147,
          "pins": [
            {
              "a": {
                "chip": "BC36",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/14",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "148": {
        "mapping": {
          "id": 148,
          "name": "fab1/13/1",
          "controllingPort": 148,
          "pins": [
            {
              "a": {
                "chip": "BC37",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/13",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "149": {
        "mapping": {
          "id": 149,
          "name": "fab1/13/2",
          "controllingPort": 149,
          "pins": [
            {
              "a": {
                "chip": "BC37",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/13",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "150": {
        "mapping": {
          "id": 150,
          "name": "fab1/13/3",
          "controllingPort": 150,
          "pins": [
            {
              "a": {
                "chip": "BC37",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/13",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "151": {
        "mapping": {
          "id": 151,
          "name": "fab1/13/4",
          "controllingPort": 151,
          "pins": [
            {
              "a": {
                "chip": "BC37",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/13",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "152": {
        "mapping": {
          "id": 152,
          "name": "fab1/15/1",
          "controllingPort": 152,
          "pins": [
            {
              "a": {
                "chip": "BC38",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/15",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "153": {
        "mapping": {
          "id": 153,
          "name": "fab1/15/2",
          "controllingPort": 153,
          "pins": [
            {
              "a": {
                "chip": "BC38",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/15",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "154": {
        "mapping": {
          "id": 154,
          "name": "fab1/15/3",
          "controllingPort": 154,
          "pins": [
            {
              "a": {
                "chip": "BC38",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/15",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "155": {
        "mapping": {
          "id": 155,
          "name": "fab1/15/4",
          "controllingPort": 155,
          "pins": [
            {
              "a": {
                "chip": "BC38",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/15",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "156": {
        "mapping": {
          "id": 156,
          "name": "fab1/16/1",
          "controllingPort": 156,
          "pins": [
            {
              "a": {
                "chip": "BC39",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/16",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "157": {
        "mapping": {
          "id": 157,
          "name": "fab1/16/2",
          "controllingPort": 157,
          "pins": [
            {
              "a": {
                "chip": "BC39",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/16",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "158": {
        "mapping": {
          "id": 158,
          "name": "fab1/16/3",
          "controllingPort": 158,
          "pins": [
            {
              "a": {
                "chip": "BC39",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/16",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "159": {
        "mapping": {
          "id": 159,
          "name": "fab1/16/4",
          "controllingPort": 159,
          "pins": [
            {
              "a": {
                "chip": "BC39",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/16",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "160": {
        "mapping": {
          "id": 160,
          "name": "fab1/17/1",
          "controllingPort": 160,
          "pins": [
            {
              "a": {
                "chip": "BC40",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/17",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "161": {
        "mapping": {
          "id": 161,
          "name": "fab1/17/2",
          "controllingPort": 161,
          "pins": [
            {
              "a": {
                "chip": "BC40",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/17",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "162": {
        "mapping": {
          "id": 162,
          "name": "fab1/17/3",
          "controllingPort": 162,
          "pins": [
            {
              "a": {
                "chip": "BC40",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/17",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "163": {
        "mapping": {
          "id": 163,
          "name": "fab1/17/4",
          "controllingPort": 163,
          "pins": [
            {
              "a": {
                "chip": "BC40",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/17",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "164": {
        "mapping": {
          "id": 164,
          "name": "fab1/18/1",
          "controllingPort": 164,
          "pins": [
            {
              "a": {
                "chip": "BC41",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/18",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "165": {
        "mapping": {
          "id": 165,
          "name": "fab1/18/2",
          "controllingPort": 165,
          "pins": [
            {
              "a": {
                "chip": "BC41",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/18",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "166": {
        "mapping": {
          "id": 166,
          "name": "fab1/18/3",
          "controllingPort": 166,
          "pins": [
            {
              "a": {
                "chip": "BC41",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/18",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "167": {
        "mapping": {
          "id": 167,
          "name": "fab1/18/4",
          "controllingPort": 167,
          "pins": [
            {
              "a": {
                "chip": "BC41",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/18",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "168": {
        "mapping": {
          "id": 168,
          "name": "fab1/20/1",
          "controllingPort": 168,
          "pins": [
            {
              "a": {
                "chip": "BC42",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/20",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "169": {
        "mapping": {
          "id": 169,
          "name": "fab1/20/2",
          "controllingPort": 169,
          "pins": [
            {
              "a": {
                "chip": "BC42",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/20",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "170": {
        "mapping": {
          "id": 170,
          "name": "fab1/20/3",
          "controllingPort": 170,
          "pins": [
            {
              "a": {
                "chip": "BC42",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/20",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "171": {
        "mapping": {
          "id": 171,
          "name": "fab1/20/4",
          "controllingPort": 171,
          "pins": [
            {
              "a": {
                "chip": "BC42",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/20",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "172": {
        "mapping": {
          "id": 172,
          "name": "fab1/19/1",
          "controllingPort": 172,
          "pins": [
            {
              "a": {
                "chip": "BC43",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/19",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "173": {
        "mapping": {
          "id": 173,
          "name": "fab1/19/2",
          "controllingPort": 173,
          "pins": [
            {
              "a": {
                "chip": "BC43",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/19",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "174": {
        "mapping": {
          "id": 174,
          "name": "fab1/19/3",
          "controllingPort": 174,
          "pins": [
            {
              "a": {
                "chip": "BC43",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/19",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "175": {
        "mapping": {
          "id": 175,
          "name": "fab1/19/4",
          "controllingPort": 175,
          "pins": [
            {
              "a": {
                "chip": "BC43",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/19",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "176": {
        "mapping": {
          "id": 176,
          "name": "fab1/37/5",
          "controllingPort": 176,
          "pins": [
            {
              "a": {
                "chip": "BC44",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/37",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "177": {
        "mapping": {
          "id": 177,
          "name": "fab1/37/6",
          "controllingPort": 177,
          "pins": [
            {
              "a": {
                "chip": "BC44",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/37",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "178": {
        "mapping": {
          "id": 178,
          "name": "fab1/37/7",
          "controllingPort": 178,
          "pins": [
            {
              "a": {
                "chip": "BC44",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/37",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "179": {
        "mapping": {
          "id": 179,
          "name": "fab1/37/8",
          "controllingPort": 179,
          "pins": [
            {
              "a": {
                "chip": "BC44",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/37",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "180": {
        "mapping": {
          "id": 180,
          "name": "fab1/22/1",
          "controllingPort": 180,
          "pins": [
            {
              "a": {
                "chip": "BC45",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/22",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "181": {
        "mapping": {
          "id": 181,
          "name": "fab1/22/2",
          "controllingPort": 181,
          "pins": [
            {
              "a": {
                "chip": "BC45",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/22",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "182": {
        "mapping": {
          "id": 182,
          "name": "fab1/22/3",
          "controllingPort": 182,
          "pins": [
            {
              "a": {
                "chip": "BC45",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/22",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "183": {
        "mapping": {
          "id": 183,
          "name": "fab1/22/4",
          "controllingPort": 183,
          "pins": [
            {
              "a": {
                "chip": "BC45",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/22",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "184": {
        "mapping": {
          "id": 184,
          "name": "fab1/38/5",
          "controllingPort": 184,
          "pins": [
            {
              "a": {
                "chip": "BC46",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/38",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "185": {
        "mapping": {
          "id": 185,
          "name": "fab1/38/6",
          "controllingPort": 185,
          "pins": [
            {
              "a": {
                "chip": "BC46",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/38",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "186": {
        "mapping": {
          "id": 186,
          "name": "fab1/38/7",
          "controllingPort": 186,
          "pins": [
            {
              "a": {
                "chip": "BC46",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/38",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "187": {
        "mapping": {
          "id": 187,
          "name": "fab1/38/8",
          "controllingPort": 187,
          "pins": [
            {
              "a": {
                "chip": "BC46",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/38",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "188": {
        "mapping": {
          "id": 188,
          "name": "fab1/39/5",
          "controllingPort": 188,
          "pins": [
            {
              "a": {
                "chip": "BC47",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "fab1/39",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC47",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC47",
                      "lane": 0
                    }
                  }
                ]
              }
          }
        }
    },
    "189": {
        "mapping": {
          "id": 189,
          "name": "fab1/39/6",
          "controllingPort": 189,
          "pins": [
            {
              "a": {
                "chip": "BC47",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "fab1/39",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC47",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC47",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "190": {
        "mapping": {
          "id": 190,
          "name": "fab1/39/7",
          "controllingPort": 190,
          "pins": [
            {
              "a": {
                "chip": "BC47",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "fab1/39",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC47",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC47",
                      "lane": 2
                    }
                  }
                ]
              }
          }
        }
    },
    "191": {
        "mapping": {
          "id": 191,
          "name": "fab1/39/8",
          "controllingPort": 191,
          "pins": [
            {
              "a": {
                "chip": "BC47",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "fab1/39",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 1
        },
        "supportedProfiles": {
          "36": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC47",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "37": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC47",
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
      "name": "BC0",
      "type": 1,
      "physicalID": 0
    },
    {
      "name": "BC1",
      "type": 1,
      "physicalID": 1
    },
    {
      "name": "BC2",
      "type": 1,
      "physicalID": 2
    },
    {
      "name": "BC3",
      "type": 1,
      "physicalID": 3
    },
    {
      "name": "BC4",
      "type": 1,
      "physicalID": 4
    },
    {
      "name": "BC5",
      "type": 1,
      "physicalID": 5
    },
    {
      "name": "BC6",
      "type": 1,
      "physicalID": 6
    },
    {
      "name": "BC7",
      "type": 1,
      "physicalID": 7
    },
    {
      "name": "BC8",
      "type": 1,
      "physicalID": 8
    },
    {
      "name": "BC9",
      "type": 1,
      "physicalID": 9
    },
    {
      "name": "BC10",
      "type": 1,
      "physicalID": 10
    },
    {
      "name": "BC11",
      "type": 1,
      "physicalID": 11
    },
    {
      "name": "BC12",
      "type": 1,
      "physicalID": 12
    },
    {
      "name": "BC13",
      "type": 1,
      "physicalID": 13
    },
    {
      "name": "BC14",
      "type": 1,
      "physicalID": 14
    },
    {
      "name": "BC15",
      "type": 1,
      "physicalID": 15
    },
    {
      "name": "BC16",
      "type": 1,
      "physicalID": 16
    },
    {
      "name": "BC17",
      "type": 1,
      "physicalID": 17
    },
    {
      "name": "BC18",
      "type": 1,
      "physicalID": 18
    },
    {
      "name": "BC19",
      "type": 1,
      "physicalID": 19
    },
    {
      "name": "BC20",
      "type": 1,
      "physicalID": 20
    },
    {
      "name": "BC21",
      "type": 1,
      "physicalID": 21
    },
    {
      "name": "BC22",
      "type": 1,
      "physicalID": 22
    },
    {
      "name": "BC23",
      "type": 1,
      "physicalID": 23
    },
    {
      "name": "BC24",
      "type": 1,
      "physicalID": 24
    },
    {
      "name": "BC25",
      "type": 1,
      "physicalID": 25
    },
    {
      "name": "BC26",
      "type": 1,
      "physicalID": 26
    },
    {
      "name": "BC27",
      "type": 1,
      "physicalID": 27
    },
    {
      "name": "BC28",
      "type": 1,
      "physicalID": 28
    },
    {
      "name": "BC29",
      "type": 1,
      "physicalID": 29
    },
    {
      "name": "BC30",
      "type": 1,
      "physicalID": 30
    },
    {
      "name": "BC31",
      "type": 1,
      "physicalID": 31
    },
    {
      "name": "BC32",
      "type": 1,
      "physicalID": 32
    },
    {
      "name": "BC33",
      "type": 1,
      "physicalID": 33
    },
    {
      "name": "BC34",
      "type": 1,
      "physicalID": 34
    },
    {
      "name": "BC35",
      "type": 1,
      "physicalID": 35
    },
    {
      "name": "BC36",
      "type": 1,
      "physicalID": 36
    },
    {
      "name": "BC37",
      "type": 1,
      "physicalID": 37
    },
    {
      "name": "BC38",
      "type": 1,
      "physicalID": 38
    },
    {
      "name": "BC39",
      "type": 1,
      "physicalID": 39
    },
    {
      "name": "BC40",
      "type": 1,
      "physicalID": 40
    },
    {
      "name": "BC41",
      "type": 1,
      "physicalID": 41
    },
    {
      "name": "BC42",
      "type": 1,
      "physicalID": 42
    },
    {
      "name": "BC43",
      "type": 1,
      "physicalID": 43
    },
    {
      "name": "BC44",
      "type": 1,
      "physicalID": 44
    },
    {
      "name": "BC45",
      "type": 1,
      "physicalID": 45
    },
    {
      "name": "BC46",
      "type": 1,
      "physicalID": 46
    },
    {
      "name": "BC47",
      "type": 1,
      "physicalID": 47
    },
    {
      "name": "fab1/1",
      "type": 3,
      "physicalID": 0
    },
    {
      "name": "fab1/2",
      "type": 3,
      "physicalID": 1
    },
    {
      "name": "fab1/3",
      "type": 3,
      "physicalID": 2
    },
    {
      "name": "fab1/4",
      "type": 3,
      "physicalID": 3
    },
    {
      "name": "fab1/5",
      "type": 3,
      "physicalID": 4
    },
    {
      "name": "fab1/6",
      "type": 3,
      "physicalID": 5
    },
    {
      "name": "fab1/7",
      "type": 3,
      "physicalID": 6
    },
    {
      "name": "fab1/8",
      "type": 3,
      "physicalID": 7
    },
    {
      "name": "fab1/9",
      "type": 3,
      "physicalID": 8
    },
    {
      "name": "fab1/10",
      "type": 3,
      "physicalID": 9
    },
    {
      "name": "fab1/11",
      "type": 3,
      "physicalID": 10
    },
    {
      "name": "fab1/12",
      "type": 3,
      "physicalID": 11
    },
    {
      "name": "fab1/13",
      "type": 3,
      "physicalID": 12
    },
    {
      "name": "fab1/14",
      "type": 3,
      "physicalID": 13
    },
    {
      "name": "fab1/15",
      "type": 3,
      "physicalID": 14
    },
    {
      "name": "fab1/16",
      "type": 3,
      "physicalID": 15
    },
    {
      "name": "fab1/17",
      "type": 3,
      "physicalID": 16
    },
    {
      "name": "fab1/18",
      "type": 3,
      "physicalID": 17
    },
    {
      "name": "fab1/19",
      "type": 3,
      "physicalID": 18
    },
    {
      "name": "fab1/20",
      "type": 3,
      "physicalID": 19
    },
    {
      "name": "fab1/21",
      "type": 3,
      "physicalID": 20
    },
    {
      "name": "fab1/22",
      "type": 3,
      "physicalID": 21
    },
    {
      "name": "fab1/23",
      "type": 3,
      "physicalID": 22
    },
    {
      "name": "fab1/24",
      "type": 3,
      "physicalID": 23
    },
    {
      "name": "fab1/25",
      "type": 3,
      "physicalID": 24
    },
    {
      "name": "fab1/26",
      "type": 3,
      "physicalID": 25
    },
    {
      "name": "fab1/27",
      "type": 3,
      "physicalID": 26
    },
    {
      "name": "fab1/28",
      "type": 3,
      "physicalID": 27
    },
    {
      "name": "fab1/29",
      "type": 3,
      "physicalID": 28
    },
    {
      "name": "fab1/30",
      "type": 3,
      "physicalID": 29
    },
    {
      "name": "fab1/31",
      "type": 3,
      "physicalID": 30
    },
    {
      "name": "fab1/32",
      "type": 3,
      "physicalID": 31
    },
    {
      "name": "fab1/33",
      "type": 3,
      "physicalID": 32
    },
    {
      "name": "fab1/34",
      "type": 3,
      "physicalID": 33
    },
    {
      "name": "fab1/35",
      "type": 3,
      "physicalID": 34
    },
    {
      "name": "fab1/36",
      "type": 3,
      "physicalID": 35
    },
    {
      "name": "fab1/37",
      "type": 3,
      "physicalID": 36
    },
    {
      "name": "fab1/38",
      "type": 3,
      "physicalID": 37
    },
    {
      "name": "fab1/39",
      "type": 3,
      "physicalID": 38
    },
    {
      "name": "fab1/40",
      "type": 3,
      "physicalID": 39
    },
    {
      "name": "fab1/41",
      "type": 3,
      "physicalID": 40
    },
    {
      "name": "fab1/42",
      "type": 3,
      "physicalID": 41
    },
    {
      "name": "fab1/43",
      "type": 3,
      "physicalID": 42
    },
    {
      "name": "fab1/44",
      "type": 3,
      "physicalID": 43
    },
    {
      "name": "fab1/45",
      "type": 3,
      "physicalID": 44
    },
    {
      "name": "fab1/46",
      "type": 3,
      "physicalID": 45
    },
    {
      "name": "fab1/47",
      "type": 3,
      "physicalID": 46
    },
    {
      "name": "fab1/48",
      "type": 3,
      "physicalID": 47
    }
  ],
  "platformSettings": {
    "1": "0c:00"
  },
  "portConfigOverrides": [
    {
      "factor": {
        "ports": [
          72,
          73,
          74,
          75,
          76,
          77,
          78,
          79,
          84,
          85,
          86,
          87,
          36,
          37,
          38,
          39,
          28,
          29,
          30,
          31,
          24,
          25,
          26,
          27,
          16,
          17,
          18,
          19,
          20,
          21,
          22,
          23,
          8,
          9,
          10,
          11,
          12,
          13,
          14,
          15,
          4,
          5,
          6,
          7,
          0,
          1,
          2,
          3,
          148,
          149,
          150,
          151,
          144,
          145,
          146,
          147,
          152,
          153,
          154,
          155,
          156,
          157,
          158,
          159,
          160,
          161,
          162,
          163,
          164,
          165,
          166,
          167,
          172,
          173,
          174,
          175,
          168,
          169,
          170,
          171,
          132,
          133,
          134,
          135,
          180,
          181,
          182,
          183,
          120,
          121,
          122,
          123,
          124,
          125,
          126,
          127,
          48,
          49,
          50,
          51,
          52,
          53,
          54,
          55,
          60,
          61,
          62,
          63,
          56,
          57,
          58,
          59,
          68,
          69,
          70,
          71,
          64,
          65,
          66,
          67,
          80,
          81,
          82,
          83,
          88,
          89,
          90,
          91,
          92,
          93,
          94,
          95,
          44,
          45,
          46,
          47,
          40,
          41,
          42,
          43,
          32,
          33,
          34,
          35,
          176,
          177,
          178,
          179,
          184,
          185,
          186,
          187,
          188,
          189,
          190,
          191,
          140,
          141,
          142,
          143,
          136,
          137,
          138,
          139,
          128,
          129,
          130,
          131,
          112,
          113,
          114,
          115,
          116,
          117,
          118,
          119,
          104,
          105,
          106,
          107,
          108,
          109,
          110,
          111,
          100,
          101,
          102,
          103,
          96,
          97,
          98,
          99
        ],
        "profiles": [
          36,
          37
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": -16,
              "pre2": 0,
              "main": 132,
              "post": 0,
              "post2": 0,
              "post3": 0
            }
          }
        ]
      }
    }
  ],
  "platformSupportedProfiles": [
    {
      "factor": {
        "profileID": 36
      },
      "profile": {
        "speed": 53125,
        "iphy": {
          "numLanes": 1,
          "modulation": 2,
          "fec": 545,
          "medium": 1,
          "interfaceMode": 41,
          "interfaceType": 41
        }
      }
    },
    {
      "factor": {
        "profileID": 37
      },
      "profile": {
        "speed": 53125,
        "iphy": {
          "numLanes": 1,
          "modulation": 2,
          "fec": 545,
          "medium": 3,
          "interfaceMode": 41,
          "interfaceType": 41
        }
      }
    }
  ]
}
)";

}
/*
 * Use configerator generated kJsonPlatformMappingStr by default.
 * Alternatively, we could programmatically generate the platform mappings
 * by using another PlatformMapping constructor as below:
 *
 * TODO(skhare) Once configerator platform mapping support is complete, remove
 * this programmatic generation logic.
 *
 * Meru400bfuPlatformMapping::Meru400bfuPlatformMapping()
 *    : PlatformMapping(buildMapping()) {}
 */

Meru400bfuPlatformMapping::Meru400bfuPlatformMapping()
    : PlatformMapping(kJsonPlatformMappingStr) {}

Meru400bfuPlatformMapping::Meru400bfuPlatformMapping(
    const std::string& platformMappingStr)
    : PlatformMapping(platformMappingStr) {}

} // namespace facebook::fboss
